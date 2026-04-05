import Foundation

struct BannerProject: Identifiable, Codable, Hashable {
    let id: UUID
    var name: String
    let createdAt: Date
    var width: UInt16
    var height: UInt16

    var filename: String { "\(id.uuidString).lsnap" }
}

final class ProjectManager: ObservableObject {
    @Published private(set) var projects: [BannerProject] = []

    static let shared = ProjectManager()

    private let fileManager = FileManager.default

    private var projectsDir: URL {
        let docs = fileManager.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let dir = docs.appendingPathComponent("LEDSnap", isDirectory: true)
        try? fileManager.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir
    }

    private var manifestURL: URL {
        projectsDir.appendingPathComponent("manifest.json")
    }

    private init() {
        loadManifest()
    }

    func lsnapURL(for project: BannerProject) -> URL {
        projectsDir.appendingPathComponent(project.filename)
    }

    @discardableResult
    func createProject(name: String, width: UInt16, height: UInt16) -> BannerProject {
        let project = BannerProject(
            id: UUID(),
            name: name.isEmpty ? "Untitled" : name,
            createdAt: Date(),
            width: width,
            height: height
        )
        projects.insert(project, at: 0)
        saveManifest()
        return project
    }

    func deleteProject(_ project: BannerProject) {
        let url = lsnapURL(for: project)
        try? fileManager.removeItem(at: url)
        projects.removeAll { $0.id == project.id }
        saveManifest()
    }

    func renameProject(_ project: BannerProject, to newName: String) {
        guard let idx = projects.firstIndex(where: { $0.id == project.id }) else { return }
        projects[idx].name = newName
        saveManifest()
    }

    private func loadManifest() {
        guard let data = try? Data(contentsOf: manifestURL),
              let list = try? JSONDecoder().decode([BannerProject].self, from: data)
        else { return }
        projects = list
    }

    private func saveManifest() {
        guard let data = try? JSONEncoder().encode(projects) else { return }
        try? data.write(to: manifestURL, options: .atomic)
    }
}
