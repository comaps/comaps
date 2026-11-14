#import "MWMStorage+UI.h"
#import "SwiftBridge.h"

@implementation MWMStorage (UI)

- (void)handleError:(NSError *)error {
  if (error.code == kStorageNotEnoughSpace) {
    [AlertPresenter.shared presentNotEnoughSpaceAlert];
  } else if (error.code == kStorageNoConnection) {
    [AlertPresenter.shared presentNoConnectionAlert];
  } else if (error.code == kStorageRoutingActive) {
    [AlertPresenter.shared presentDeleteMapProhibitedAlert];
  } else {
    NSAssert(NO, @"Unknown error code");
  }
}

- (void)downloadNode:(NSString *)countryId {
  [self downloadNode:countryId onSuccess:nil];
}

- (void)downloadNode:(NSString *)countryId onSuccess:(MWMVoidBlock)success {
  NSError *error;
  [self downloadNode:countryId error:&error];
  if (error) {
    if (error.code == kStorageCellularForbidden) {
      __weak __typeof(self) ws = self;
      [AlertPresenter.shared presentNoWiFiAlertOnConfirm:^{
        [self enableCellularDownload:YES];
        [ws downloadNode:countryId];
      } onCancel:nil];
    } else {
      [self handleError:error];
    }
    return;
  }
  if (success) {
    success();
  }
}

- (void)updateNode:(NSString *)countryId {
  [self updateNode:countryId onCancel:nil];
}

- (void)updateNode:(NSString *)countryId onCancel:(MWMVoidBlock)cancel {
  NSError *error;
  [self updateNode:countryId error:&error];
  if (error) {
    if (error.code == kStorageCellularForbidden) {
      __weak __typeof(self) ws = self;
      [AlertPresenter.shared presentNoWiFiAlertOnConfirm:^{
        [self enableCellularDownload:YES];
        [ws updateNode:countryId onCancel:cancel];
      } onCancel:cancel];
    } else {
      [self handleError:error];
      if (cancel) {
        cancel();
      }
    }
  }
}

- (void)deleteNode:(NSString *)countryId {
  [self deleteNode:countryId ignoreUnsavedEdits:NO];
}

- (void)deleteNode:(NSString *)countryId ignoreUnsavedEdits:(BOOL)force {
  NSError *error;
  [self deleteNode:countryId ignoreUnsavedEdits:force error:&error];
  if (error) {
    __weak __typeof(self) ws = self;
    if (error.code == kStorageCellularForbidden) {
      [AlertPresenter.shared presentNoWiFiAlertOnConfirm:^{
        [self enableCellularDownload:YES];
        [ws deleteNode:countryId];
      } onCancel:nil];
    } else if (error.code == kStorageHaveUnsavedEdits) {
      [AlertPresenter.shared presentUnsavedEditsAlertOnDelete:^ {
        [ws deleteNode:countryId ignoreUnsavedEdits:YES];
      }];
    } else {
      [self handleError:error];
    }
  }
}

- (void)downloadNodes:(NSArray<NSString *> *)countryIds onSuccess:(nullable MWMVoidBlock)success {
  NSError *error;
  [self downloadNodes:countryIds error:&error];
  if (error) {
    if (error.code == kStorageCellularForbidden) {
      __weak __typeof(self) ws = self;
      [AlertPresenter.shared presentNoWiFiAlertOnConfirm:^{
        [self enableCellularDownload:YES];
        [ws downloadNodes:countryIds onSuccess:success];
      } onCancel:nil];
    } else {
      [self handleError:error];
    }
    return;
  }
  if (success) {
    success();
  }
}

@end
