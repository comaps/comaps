import CoreBluetooth
import UIKit

final class BluetoothDevicesViewController: MWMViewController {
  private struct BluetoothDevice: Equatable {
    let identifier: UUID
    var name: String
    var rssi: NSNumber
  }

  private let serviceUUID: CBUUID
  private let tableView = UITableView(frame: .zero, style: .plain)
  private var placeholderView: PlaceholderView?
  private var centralManager: CBCentralManager!
  private var devices: [BluetoothDevice] = []

  init(serviceUUID: CBUUID) {
    self.serviceUUID = serviceUUID
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("bluetooth_devices")
    setupNavigationItem()
    setupTableView()
    setupPlaceholderView()
    centralManager = CBCentralManager(delegate: self, queue: .main)
  }

  deinit {
    centralManager?.stopScan()
  }

  private func setupNavigationItem() {
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .close,
                                                       target: self,
                                                       action: #selector(close))
  }

  private func setupTableView() {
    tableView.setStyle(.background)
    tableView.register(UITableViewCell.self, forCellReuseIdentifier: UITableViewCell.className())
    tableView.dataSource = self
    tableView.tableFooterView = UIView()

    view.addSubview(tableView)
    tableView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      tableView.topAnchor.constraint(equalTo: view.topAnchor),
      tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      tableView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  private func setupPlaceholderView() {
    showPlaceholder(title: L("bluetooth_devices"), subtitle: L("bluetooth_devices_searching"), hasActivityIndicator: true)
  }

  private func startScanning() {
    devices.removeAll()
    tableView.reloadData()
    showPlaceholder(title: L("bluetooth_devices"), subtitle: L("bluetooth_devices_searching"), hasActivityIndicator: true)
    centralManager.scanForPeripherals(withServices: [serviceUUID], options: [CBCentralManagerScanOptionAllowDuplicatesKey: true])
  }

  private func stopScanning(with title: String, subtitle: String?) {
    centralManager.stopScan()
    showPlaceholder(title: title, subtitle: subtitle, hasActivityIndicator: false)
  }

  private func showPlaceholder(title: String, subtitle: String?, hasActivityIndicator: Bool) {
    placeholderView?.removeFromSuperview()
    let placeholderView = PlaceholderView(title: title, subtitle: subtitle, hasActivityIndicator: hasActivityIndicator)
    self.placeholderView = placeholderView
    view.addSubview(placeholderView)
    placeholderView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      placeholderView.topAnchor.constraint(equalTo: view.topAnchor),
      placeholderView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      placeholderView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      placeholderView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  private func hidePlaceholder() {
    placeholderView?.removeFromSuperview()
    placeholderView = nil
  }

  private func upsertDevice(identifier: UUID, name: String?, rssi: NSNumber) {
    let displayName = name?.isEmpty == false ? name! : L("unknown")
    if let index = devices.firstIndex(where: { $0.identifier == identifier }) {
      devices[index].name = displayName
      devices[index].rssi = rssi
      tableView.reloadRows(at: [IndexPath(row: index, section: 0)], with: .none)
    } else {
      devices.append(BluetoothDevice(identifier: identifier, name: displayName, rssi: rssi))
      hidePlaceholder()
      tableView.insertRows(at: [IndexPath(row: devices.count - 1, section: 0)], with: .automatic)
    }
  }

  @objc private func close() {
    dismiss(animated: true)
  }
}

extension BluetoothDevicesViewController: CBCentralManagerDelegate {
  func centralManagerDidUpdateState(_ central: CBCentralManager) {
    switch central.state {
    case .poweredOn:
      startScanning()
    case .poweredOff:
      stopScanning(with: L("bluetooth_devices_unavailable"), subtitle: L("bluetooth_devices_powered_off"))
    case .unauthorized:
      stopScanning(with: L("bluetooth_devices_unavailable"), subtitle: L("bluetooth_devices_unauthorized"))
    case .unsupported:
      stopScanning(with: L("bluetooth_devices_unavailable"), subtitle: L("bluetooth_devices_unsupported"))
    case .resetting, .unknown:
      showPlaceholder(title: L("bluetooth_devices"), subtitle: L("bluetooth_devices_searching"), hasActivityIndicator: true)
    @unknown default:
      stopScanning(with: L("bluetooth_devices_unavailable"), subtitle: nil)
    }
  }

  func centralManager(_ central: CBCentralManager,
                      didDiscover peripheral: CBPeripheral,
                      advertisementData: [String : Any],
                      rssi RSSI: NSNumber) {
    let localName = advertisementData[CBAdvertisementDataLocalNameKey] as? String
    upsertDevice(identifier: peripheral.identifier, name: peripheral.name ?? localName, rssi: RSSI)
  }
}

extension BluetoothDevicesViewController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    devices.count
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(withIdentifier: UITableViewCell.className(), for: indexPath)
    let device = devices[indexPath.row]
    var content = cell.defaultContentConfiguration()
    content.text = device.name
    content.secondaryText = String(format: "RSSI: %@ dBm", device.rssi)
    cell.contentConfiguration = content
    cell.selectionStyle = .none
    return cell
  }
}
