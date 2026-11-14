extension NetworkPolicy {
  private var isTempPermissionValid: Bool {
    permissionExpirationDate > Date()
  }

  private func askPermission(completion: @escaping (Bool) -> Void) {
    AlertPresenter.shared.presentMobileInternetAlert(onAlways: {
      self.permission = .always
      completion(true)
    }, onToday: {
      self.permission = .today
      completion(true)
    }, onNotToday: {
      self.permission = .notToday
      completion(false)
    })
  }

  @objc
  func callOnlineApi(_ onlineCall: @escaping (Bool) -> Void) {
    callOnlineApi(onlineCall, forceAskPermission: false)
  }

  @objc
  func callOnlineApi(_ onlineCall: @escaping (Bool) -> Void, forceAskPermission: Bool) {
    switch connectionType {
    case .none:
      onlineCall(false)
    case .wifi:
      onlineCall(true)
    case .cellular:
      switch permission {
      case .ask:
        askPermission(completion: onlineCall)
      case .always:
        onlineCall(true)
      case .never:
        if forceAskPermission {
          askPermission(completion: onlineCall)
        } else {
          onlineCall(false)
        }
      case .today:
        if isTempPermissionValid {
          onlineCall(true)
        } else {
          askPermission(completion: onlineCall)
        }
      case .notToday:
        if !isTempPermissionValid || forceAskPermission {
          askPermission(completion: onlineCall)
        } else {
          onlineCall(false)
        }
      @unknown default:
        onlineCall(false)
      }
    @unknown default:
      onlineCall(false)
    }
  }
}
