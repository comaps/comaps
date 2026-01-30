import Combine

/// The settings
class RegionalManager {
    // MARK: Properties
    
    /// The current distance unit
    static var root: Region {
        get {
            let rootId = Storage.shared().getRootId()
            return Region(id: rootId, name: "World", subregions: findSubregions(for: rootId))
        }
    }
    
    static func delete(for regionId: Region.ID) {
        Storage.shared().deleteNode(regionId)
    }
    
    static func cancel(for regionId: Region.ID) {
        Storage.shared().cancelDownloadNode(regionId)
    }
    
    static func update(for regionId: Region.ID) {
        Storage.shared().updateNode(regionId)
    }
    
    static func download(for regionId: Region.ID) {
        Storage.shared().downloadNode(regionId)
    }
    
    static func state(for regionId: Region.ID) -> Region.State {
        let attributes = Storage.shared().attributes(forCountry: regionId)
        if attributes.nodeStatus == .onDiskOutOfDate {
            return .downloadedButOutdated
        } else if attributes.nodeStatus == .onDisk || attributes.nodeStatus == .applying {
            return .downloaded
        } else if attributes.nodeStatus == .applying {
            return .downloading(1)
        } else if attributes.nodeStatus == .inQueue || attributes.nodeStatus == .downloading {
            return .downloading(Double(attributes.downloadingSize))
        } else {
            return .notDownloaded
        }
    }
    
    private static func findSubregions(for regionId: Region.ID) -> [Region] {
        var subregions: [Region] = []
        for id in Storage.shared().allCountries(withParent: regionId) {
            let attributes = Storage.shared().attributes(forCountry: id)
            let name = attributes.nodeName
            let description = attributes.nodeDescription
            var subregion = Region(id: id, name: name, description: description)
            subregion.downloadedSize = Measurement(value: Double(attributes.downloadedSize), unit: .bytes)
            subregion.downloadingSize = Measurement(value: Double(attributes.totalSize), unit: .bytes)
            subregion.subregions = findSubregions(for: id)
            subregions.append(subregion)
        }
        return subregions.sorted()
    }
}
