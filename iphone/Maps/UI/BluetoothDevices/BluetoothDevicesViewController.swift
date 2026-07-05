import CoreBluetooth
import SwiftUI

private enum RadarNavigationBluetooth {
  static let navigationCharacteristicUUID = CBUUID(string: "00000002-b691-470d-8439-e8a21d4caef5")
  static let defaultDeviceName = "Radar"
  static let maxPacketBytes = 106
  static let maxTextBytes = 40
  static let sequenceDefaultsKey = "RadarNavigationBluetoothSequence"
}

struct BluetoothDevice: Identifiable, Equatable {
  let id: UUID
  var name: String
  var rssi: NSNumber
  var isConnected: Bool = false
  var isReadyForUpdates: Bool = false
}

enum BluetoothScannerState: Equatable {
  case searching
  case unavailable(title: String, subtitle: String?)
}

private struct RadarNavigationUpdate {
  enum State: UInt32 {
    case unspecified = 0
    case idle = 1
    case active = 2
    case rerouting = 3
    case arrived = 4
    case cleared = 5
  }

  enum Maneuver: UInt32 {
    case unspecified = 0
    case unknown = 1
    case straight = 2
    case turnLeft = 3
    case turnRight = 4
    case slightLeft = 5
    case slightRight = 6
    case sharpLeft = 7
    case sharpRight = 8
    case uTurn = 9
    case roundabout = 10
    case destination = 11
  }

  let sequence: UInt32
  let state: State
  let maneuver: Maneuver
  let distanceMeters: UInt32
  let etaSeconds: UInt32
  let primary: String
  let secondary: String
}

private enum RadarNavigationEncoder {
  static func encode(_ update: RadarNavigationUpdate) -> Data? {
    var data = Data()
    appendVarintField(number: 1, value: UInt64(update.sequence), to: &data)
    appendVarintField(number: 2, value: UInt64(update.state.rawValue), to: &data)
    appendVarintField(number: 3, value: UInt64(update.maneuver.rawValue), to: &data)
    appendVarintField(number: 4, value: UInt64(update.distanceMeters), to: &data)
    appendVarintField(number: 5, value: UInt64(update.etaSeconds), to: &data)
    appendStringField(number: 6, value: update.primary, to: &data)
    appendStringField(number: 7, value: update.secondary, to: &data)
    return data.count <= RadarNavigationBluetooth.maxPacketBytes ? data : nil
  }

  private static func appendVarintField(number: UInt8, value: UInt64, to data: inout Data) {
    guard value != 0 else { return }
    data.append(number << 3)
    appendVarint(value, to: &data)
  }

  private static func appendStringField(number: UInt8, value: String, to data: inout Data) {
    let truncated = value.truncatedToUTF8ByteCount(RadarNavigationBluetooth.maxTextBytes)
    guard !truncated.isEmpty else { return }

    let bytes = Array(truncated.utf8)
    data.append((number << 3) | 2)
    appendVarint(UInt64(bytes.count), to: &data)
    data.append(contentsOf: bytes)
  }

  private static func appendVarint(_ value: UInt64, to data: inout Data) {
    var value = value
    while value >= 0x80 {
      data.append(UInt8(value & 0x7F) | 0x80)
      value >>= 7
    }
    data.append(UInt8(value))
  }
}

private extension String {
  func truncatedToUTF8ByteCount(_ maxBytes: Int) -> String {
    var result = ""
    var byteCount = 0
    for character in self {
      let characterByteCount = String(character).utf8.count
      guard byteCount + characterByteCount <= maxBytes else { break }
      result.append(character)
      byteCount += characterByteCount
    }
    return result
  }
}

private extension UInt32 {
  static func clamping(_ value: Double) -> UInt32 {
    guard value.isFinite else { return 0 }
    return UInt32(min(Double(UInt32.max), max(0, value.rounded())))
  }
}

private extension RouteInfo {
  var radarNavigationUpdateState: RadarNavigationUpdate.State {
    carDirection == .reachedYourDestination ? .arrived : .active
  }

  var radarManeuver: RadarNavigationUpdate.Maneuver {
    switch carDirection {
    case .goStraight, .startAtEndOfStreet:
      return .straight
    case .turnRight, .exitHighwayToRight:
      return .turnRight
    case .turnSharpRight:
      return .sharpRight
    case .turnSlightRight:
      return .slightRight
    case .turnLeft, .exitHighwayToLeft:
      return .turnLeft
    case .turnSharpLeft:
      return .sharpLeft
    case .turnSlightLeft:
      return .slightLeft
    case .uTurnLeft, .uTurnRight:
      return .uTurn
    case .enterRoundAbout, .leaveRoundAbout, .stayOnRoundAbout:
      return .roundabout
    case .reachedYourDestination:
      return .destination
    case .none:
      return .unknown
    }
  }

  var radarDistanceToTurnMeters: UInt32 {
    UInt32.clamping(Measurement(value: distanceToTurn, unit: turnUnits).converted(to: .meters).value)
  }

  var radarEtaSeconds: UInt32 {
    UInt32.clamping(timeToTarget)
  }

  var radarPrimaryText: String {
    if carDirection == .reachedYourDestination {
      return "Navigation"
    }

    let variants = NavigationInstructionFormatter.instructionVariants(roadName: roadName,
                                                                      roadRef: roadRef,
                                                                      junctionRef: junctionRef,
                                                                      destinationRef: destinationRef,
                                                                      destination: destination,
                                                                      isLink: isLink)
    return variants.first ?? carDirection.radarDisplayText
  }

  var radarSecondaryText: String {
    if carDirection == .reachedYourDestination {
      return "Destination"
    }
    if roundExitNumber > 0 {
      return "Exit \(roundExitNumber)"
    }
    return carDirection.radarDisplayText
  }
}

private extension CarDirection {
  var radarDisplayText: String {
    switch self {
    case .goStraight:
      return "Continue"
    case .turnRight:
      return "Turn right"
    case .turnSharpRight:
      return "Sharp right"
    case .turnSlightRight:
      return "Slight right"
    case .turnLeft:
      return "Turn left"
    case .turnSharpLeft:
      return "Sharp left"
    case .turnSlightLeft:
      return "Slight left"
    case .uTurnLeft, .uTurnRight:
      return "U-turn"
    case .enterRoundAbout, .stayOnRoundAbout:
      return "Roundabout"
    case .leaveRoundAbout:
      return "Exit roundabout"
    case .startAtEndOfStreet:
      return "Start route"
    case .reachedYourDestination:
      return "Arrive"
    case .exitHighwayToLeft:
      return "Exit left"
    case .exitHighwayToRight:
      return "Exit right"
    case .none:
      return "Continue"
    }
  }
}

final class BluetoothDevicesViewModel: NSObject, ObservableObject {
  @Published private(set) var devices: [BluetoothDevice] = []
  @Published private(set) var state: BluetoothScannerState = .searching

  private static var sharedInstance: BluetoothDevicesViewModel?

  private let serviceUUID: CBUUID
  private let characteristicUUID: CBUUID
  private var centralManager: CBCentralManager?
  private var peripherals: [UUID: CBPeripheral] = [:]
  private var navigationCharacteristics: [UUID: CBCharacteristic] = [:]
  private var lastNavigationPayload: Data?
  private var sequence = BluetoothDevicesViewModel.initialSequence()

  static func shared(serviceUUID: CBUUID) -> BluetoothDevicesViewModel {
    if let sharedInstance {
      return sharedInstance
    }

    let viewModel = BluetoothDevicesViewModel(serviceUUID: serviceUUID,
                                             characteristicUUID: RadarNavigationBluetooth.navigationCharacteristicUUID)
    sharedInstance = viewModel
    return viewModel
  }

  private init(serviceUUID: CBUUID, characteristicUUID: CBUUID) {
    self.serviceUUID = serviceUUID
    self.characteristicUUID = characteristicUUID
    super.init()
    RoutingManager.routingManager.add(self)
    centralManager = CBCentralManager(delegate: self, queue: .main)
  }

  private static func initialSequence() -> UInt32 {
    let storedSequence = (UserDefaults.standard.object(forKey: RadarNavigationBluetooth.sequenceDefaultsKey) as? NSNumber)?
      .uint32Value ?? 0
    let timeSequence = UInt32(Date().timeIntervalSince1970)
    return max(storedSequence, timeSequence)
  }

  deinit {
    RoutingManager.routingManager.remove(self)
    centralManager?.stopScan()
    for peripheral in peripherals.values {
      centralManager?.cancelPeripheralConnection(peripheral)
    }
  }

  private func startScanning(resetDevices: Bool = false) {
    if resetDevices {
      devices.removeAll()
      peripherals.removeAll()
      navigationCharacteristics.removeAll()
    }
    state = .searching
    centralManager?.scanForPeripherals(withServices: nil,
                                       options: [CBCentralManagerScanOptionAllowDuplicatesKey: true])
  }

  private func stopScanning(title: String, subtitle: String?) {
    centralManager?.stopScan()
    state = .unavailable(title: title, subtitle: subtitle)
  }

  private func upsertDevice(identifier: UUID,
                            name: String?,
                            rssi: NSNumber,
                            isConnected: Bool? = nil,
                            isReadyForUpdates: Bool? = nil) {
    let displayName = name?.isEmpty == false ? name! : L("unknown")
    if let index = devices.firstIndex(where: { $0.id == identifier }) {
      devices[index].name = displayName
      devices[index].rssi = rssi
      if let isConnected {
        devices[index].isConnected = isConnected
      }
      if let isReadyForUpdates {
        devices[index].isReadyForUpdates = isReadyForUpdates
      }
    } else {
      devices.append(BluetoothDevice(id: identifier,
                                     name: displayName,
                                     rssi: rssi,
                                     isConnected: isConnected ?? false,
                                     isReadyForUpdates: isReadyForUpdates ?? false))
    }
  }

  private func updateConnectionState(for peripheral: CBPeripheral,
                                     isConnected: Bool,
                                     isReadyForUpdates: Bool? = nil) {
    upsertDevice(identifier: peripheral.identifier,
                 name: peripheral.name,
                 rssi: devices.first(where: { $0.id == peripheral.identifier })?.rssi ?? NSNumber(value: 0),
                 isConnected: isConnected,
                 isReadyForUpdates: isReadyForUpdates)
  }

  private func connectIfNeeded(_ peripheral: CBPeripheral) {
    guard peripheral.state == .disconnected else { return }
    centralManager?.connect(peripheral, options: nil)
  }

  private func sendCurrentNavigationUpdate() {
    let manager = RoutingManager.routingManager
    if manager.isRouteFinished {
      sendStaticNavigationUpdate(state: .arrived,
                                 maneuver: .destination,
                                 primary: "Navigation",
                                 secondary: "Destination")
      return
    }

    guard manager.isOnRoute else {
      sendStaticNavigationUpdate(state: .idle,
                                 maneuver: .unknown,
                                 primary: "",
                                 secondary: "")
      return
    }

    guard let routeInfo = manager.routeInfo else {
      sendStaticNavigationUpdate(state: .rerouting,
                                 maneuver: .unknown,
                                 primary: "Navigation",
                                 secondary: "Please wait")
      return
    }

    sendNavigationUpdate(RadarNavigationUpdate(sequence: nextSequence(),
                                               state: routeInfo.radarNavigationUpdateState,
                                               maneuver: routeInfo.radarManeuver,
                                               distanceMeters: routeInfo.radarDistanceToTurnMeters,
                                               etaSeconds: routeInfo.radarEtaSeconds,
                                               primary: routeInfo.radarPrimaryText,
                                               secondary: routeInfo.radarSecondaryText))
  }

  private func sendClearedNavigationUpdate() {
    sendStaticNavigationUpdate(state: .cleared,
                               maneuver: .unknown,
                               primary: "",
                               secondary: "")
  }

  private func sendStaticNavigationUpdate(state: RadarNavigationUpdate.State,
                                          maneuver: RadarNavigationUpdate.Maneuver,
                                          primary: String,
                                          secondary: String) {
    sendNavigationUpdate(RadarNavigationUpdate(sequence: nextSequence(),
                                               state: state,
                                               maneuver: maneuver,
                                               distanceMeters: 0,
                                               etaSeconds: 0,
                                               primary: primary,
                                               secondary: secondary))
  }

  private func sendNavigationUpdate(_ update: RadarNavigationUpdate) {
    guard let payload = RadarNavigationEncoder.encode(update) else { return }
    lastNavigationPayload = payload
    sendPayload(payload)
  }

  private func sendPayload(_ payload: Data) {
    for (identifier, characteristic) in navigationCharacteristics {
      guard let peripheral = peripherals[identifier], peripheral.state == .connected else { continue }
      write(payload, to: characteristic, on: peripheral)
    }
  }

  private func write(_ data: Data, to characteristic: CBCharacteristic, on peripheral: CBPeripheral) {
    guard data.count <= RadarNavigationBluetooth.maxPacketBytes else { return }
    guard data.count <= peripheral.maximumWriteValueLength(for: .withResponse) else { return }
    peripheral.writeValue(data, for: characteristic, type: .withResponse)
  }

  private func nextSequence() -> UInt32 {
    let current = sequence
    sequence &+= 1
    UserDefaults.standard.set(Int(sequence), forKey: RadarNavigationBluetooth.sequenceDefaultsKey)
    return current
  }
}

extension BluetoothDevicesViewModel: CBCentralManagerDelegate {
  func centralManagerDidUpdateState(_ central: CBCentralManager) {
    switch central.state {
    case .poweredOn:
      startScanning()
    case .poweredOff:
      stopScanning(title: L("bluetooth_devices_unavailable"), subtitle: L("bluetooth_devices_powered_off"))
    case .unauthorized:
      stopScanning(title: L("bluetooth_devices_unavailable"), subtitle: L("bluetooth_devices_unauthorized"))
    case .unsupported:
      stopScanning(title: L("bluetooth_devices_unavailable"), subtitle: L("bluetooth_devices_unsupported"))
    case .resetting, .unknown:
      state = .searching
    @unknown default:
      stopScanning(title: L("bluetooth_devices_unavailable"), subtitle: nil)
    }
  }

  func centralManager(_ central: CBCentralManager,
                      didDiscover peripheral: CBPeripheral,
                      advertisementData: [String: Any],
                      rssi RSSI: NSNumber) {
    let localName = advertisementData[CBAdvertisementDataLocalNameKey] as? String
    let advertisedServices = advertisementData[CBAdvertisementDataServiceUUIDsKey] as? [CBUUID] ?? []
    let displayName = peripheral.name ?? localName
    let matchesRadarDevice = advertisedServices.contains(serviceUUID) ||
      displayName?.range(of: RadarNavigationBluetooth.defaultDeviceName, options: .caseInsensitive) != nil
    guard matchesRadarDevice else { return }

    peripherals[peripheral.identifier] = peripheral
    peripheral.delegate = self
    upsertDevice(identifier: peripheral.identifier,
                 name: displayName,
                 rssi: RSSI)
    connectIfNeeded(peripheral)
  }

  func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
    updateConnectionState(for: peripheral, isConnected: true, isReadyForUpdates: false)
    peripheral.discoverServices([serviceUUID])
  }

  func centralManager(_ central: CBCentralManager,
                      didDisconnectPeripheral peripheral: CBPeripheral,
                      error: Error?) {
    navigationCharacteristics[peripheral.identifier] = nil
    updateConnectionState(for: peripheral, isConnected: false, isReadyForUpdates: false)
    connectIfNeeded(peripheral)
  }

  func centralManager(_ central: CBCentralManager,
                      didFailToConnect peripheral: CBPeripheral,
                      error: Error?) {
    updateConnectionState(for: peripheral, isConnected: false, isReadyForUpdates: false)
  }
}

extension BluetoothDevicesViewModel: CBPeripheralDelegate {
  func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
    guard error == nil else { return }
    peripheral.services?
      .filter { $0.uuid == serviceUUID }
      .forEach { peripheral.discoverCharacteristics([characteristicUUID], for: $0) }
  }

  func peripheral(_ peripheral: CBPeripheral,
                  didDiscoverCharacteristicsFor service: CBService,
                  error: Error?) {
    guard error == nil, service.uuid == serviceUUID else { return }
    guard let characteristic = service.characteristics?.first(where: {
      $0.uuid == characteristicUUID && $0.properties.contains(.write)
    }) else {
      updateConnectionState(for: peripheral, isConnected: true, isReadyForUpdates: false)
      return
    }

    navigationCharacteristics[peripheral.identifier] = characteristic
    updateConnectionState(for: peripheral, isConnected: true, isReadyForUpdates: true)
    centralManager?.stopScan()

    if let lastNavigationPayload {
      sendPayload(lastNavigationPayload)
    } else {
      sendCurrentNavigationUpdate()
    }
  }
}

extension BluetoothDevicesViewModel: RoutingManagerListener {
  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps limit: Double) {}

  func processRouteBuilderEvent(with code: RouterResultCode, countries: [String]) {
    DispatchQueue.main.async { [weak self] in
      switch code {
      case .noError, .hasWarnings:
        self?.sendCurrentNavigationUpdate()
      case .cancelled:
        self?.sendClearedNavigationUpdate()
      default:
        self?.sendClearedNavigationUpdate()
      }
    }
  }

  func didLocationUpdate(_ notifications: [String]) {
    DispatchQueue.main.async { [weak self] in
      self?.sendCurrentNavigationUpdate()
    }
  }
}

struct BluetoothDevicesView: View {
  @ObservedObject var viewModel: BluetoothDevicesViewModel
  let onClose: () -> Void

  var body: some View {
    Group {
      if viewModel.devices.isEmpty {
        placeholderView
      } else {
        deviceList
      }
    }
    .navigationTitle(L("bluetooth_devices"))
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItem(placement: .navigationBarTrailing) {
        Button(action: onClose) {
          Image(systemName: "xmark")
        }
        .accessibilityLabel(Text(L("close")))
      }
    }
  }

  private var deviceList: some View {
    List(viewModel.devices) { device in
      VStack(alignment: .leading, spacing: 4) {
        HStack {
          Text(device.name)
            .font(.body)
          Spacer()
          if device.isReadyForUpdates {
            Image(systemName: "arrow.triangle.2.circlepath.circle.fill")
              .foregroundStyle(.green)
          } else if device.isConnected {
            Image(systemName: "checkmark.circle.fill")
              .foregroundStyle(.secondary)
          }
        }
        Text("RSSI: \(device.rssi) dBm")
          .font(.footnote)
          .foregroundStyle(.secondary)
      }
      .padding(.vertical, 4)
    }
  }

  private var placeholderView: some View {
    VStack(spacing: 8) {
      if viewModel.state == .searching {
        ProgressView()
          .padding(.bottom, 4)
      }
      Text(placeholderTitle)
        .font(.body.weight(.medium))
        .multilineTextAlignment(.center)
      if let subtitle = placeholderSubtitle {
        Text(subtitle)
          .font(.footnote)
          .foregroundStyle(.secondary)
          .multilineTextAlignment(.center)
      }
    }
    .padding(.horizontal, 32)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
  }

  private var placeholderTitle: String {
    switch viewModel.state {
    case .searching:
      return L("bluetooth_devices")
    case let .unavailable(title, _):
      return title
    }
  }

  private var placeholderSubtitle: String? {
    switch viewModel.state {
    case .searching:
      return L("bluetooth_devices_searching")
    case let .unavailable(_, subtitle):
      return subtitle
    }
  }
}

final class BluetoothDevicesViewController: UIHostingController<BluetoothDevicesView> {
  private let viewModel: BluetoothDevicesViewModel

  init(serviceUUID: CBUUID) {
    let viewModel = BluetoothDevicesViewModel.shared(serviceUUID: serviceUUID)
    self.viewModel = viewModel
    super.init(rootView: BluetoothDevicesView(viewModel: viewModel, onClose: {}))
    rootView = BluetoothDevicesView(viewModel: viewModel) { [weak self] in
      self?.close()
    }
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func close() {
    dismiss(animated: true)
  }
}
