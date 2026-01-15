NS_SWIFT_NAME(SettingsBridge)
@interface MWMSettings : NSObject

+ (BOOL)buildings3dViewEnabled;
+ (void)setBuildings3dViewEnabled:(BOOL)buildings3dViewEnabled;

+ (BOOL)perspectiveViewEnabled;
+ (void)setPerspectiveViewEnabled:(BOOL)perspectiveViewEnabled;

+ (BOOL)autoZoomEnabled;
+ (void)setAutoZoomEnabled:(BOOL)autoZoomEnabled;

+ (BOOL)autoDownloadEnabled;
+ (void)setAutoDownloadEnabled:(BOOL)autoDownloadEnabled;

+ (MWMUnits)measurementUnits;
+ (void)setMeasurementUnits:(MWMUnits)measurementUnits;

+ (BOOL)zoomButtonsEnabled;
+ (void)setZoomButtonsEnabled:(BOOL)zoomButtonsEnabled;

+ (BOOL)compassCalibrationEnabled;
+ (void)setCompassCalibrationEnabled:(BOOL)compassCalibrationEnabled;

+ (MWMTheme)theme;
+ (void)setTheme:(MWMTheme)theme;

+ (NSInteger)powerManagement;
+ (void)setPowerManagement:(NSInteger)powerManagement;

+ (BOOL)routingDisclaimerApproved;
+ (void)setRoutingDisclaimerApproved;

+ (NSString *)spotlightLocaleLanguageId;
+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId;

+ (BOOL)largeFontSize;
+ (void)setLargeFontSize:(BOOL)largeFontSize;

+ (NSDictionary<NSString *, NSString *> *)availableMapLanguages;
+ (NSString *)mapLanguageCode;
+ (void)setMapLanguageCode:(NSString *)mapLanguageCode;

+ (BOOL)mapLanguageLimitAlternativesToLocal;
+ (void)setMapLanguageLimitAlternativesToLocal:(BOOL)mapLanguageLimitAlternativesToLocal;

+ (BOOL)transliteration;
+ (void)setTransliteration:(BOOL)transliteration;

+ (BOOL)isTrackWarningAlertShown;
+ (void)setTrackWarningAlertShown:(BOOL)shown;

+ (NSString *)donateUrl;

+ (BOOL)iCLoudSynchronizationEnabled;
+ (void)setICLoudSynchronizationEnabled:(BOOL)iCLoudSyncEnabled;

+ (void)initializeLogging;
+ (BOOL)isFileLoggingEnabled;
+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled;
+ (NSInteger)logFileSize;

@end
