import SwiftUI

@main
struct LEDSnapApp: App {
    var body: some Scene {
        WindowGroup {
            HomeView()
        }
    }
}

struct HomeView: View {
    @ObservedObject private var projects = ProjectManager.shared
    @State private var showNewSheet = false
    @State private var showDemo = false

    var body: some View {
        NavigationStack {
            List {
                Section {
                    Button {
                        showDemo = true
                    } label: {
                        Label("Live Demo", systemImage: "sparkles")
                    }
                }

                Section("My Banners") {
                    if projects.projects.isEmpty {
                        Text("No banners yet. Tap + to create one.")
                            .foregroundColor(.secondary)
                            .font(.caption)
                    }

                    ForEach(projects.projects) { project in
                        NavigationLink(value: project) {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(project.name)
                                    .font(.body.bold())
                                Text("\(project.width)x\(project.height) - \(project.createdAt.formatted(date: .abbreviated, time: .shortened))")
                                    .font(.caption2)
                                    .foregroundColor(.secondary)
                            }
                        }
                        .swipeActions(edge: .trailing) {
                            Button(role: .destructive) {
                                projects.deleteProject(project)
                            } label: {
                                Label("Delete", systemImage: "trash")
                            }
                        }
                        .contextMenu {
                            Button {
                                let mgr = ProjectManager.shared
                                let url = mgr.lsnapURL(for: project)
                                showBanner(source: .file(url, width: project.width, height: project.height))
                            } label: {
                                Label("Present Banner", systemImage: "play.fill")
                            }
                        }
                    }
                }
            }
            .navigationTitle("LEDSnap!")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        showNewSheet = true
                    } label: {
                        Image(systemName: "plus")
                    }
                }
            }
            .navigationDestination(for: BannerProject.self) { project in
                CanvasView(project: project)
            }
            .sheet(isPresented: $showNewSheet) {
                NewBannerSheet()
            }
            .fullScreenCover(isPresented: $showDemo) {
                BannerView(source: .demo)
            }
        }
    }

    @State private var bannerSource: BannerSource?
    @State private var showBannerCover = false

    private func showBanner(source: BannerSource) {
        bannerSource = source
        showBannerCover = true
    }
}

struct NewBannerSheet: View {
    @ObservedObject private var projects = ProjectManager.shared
    @Environment(\.dismiss) private var dismiss

    @State private var name = ""
    @State private var selectedSize = 0

    private let sizes: [(String, UInt16, UInt16)] = [
        ("32x16 (Standard)", 32, 16),
        ("64x16 (Wide)", 64, 16),
        ("64x32 (Large)", 64, 32),
        ("16x16 (Square)", 16, 16),
    ]

    var body: some View {
        NavigationStack {
            Form {
                Section("Banner Name") {
                    TextField("My Banner", text: $name)
                }

                Section("Grid Size") {
                    Picker("Size", selection: $selectedSize) {
                        ForEach(0..<sizes.count, id: \.self) { i in
                            Text(sizes[i].0).tag(i)
                        }
                    }
                    .pickerStyle(.inline)
                    .labelsHidden()
                }
            }
            .navigationTitle("New Banner")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Create") {
                        let s = sizes[selectedSize]
                        projects.createProject(name: name, width: s.1, height: s.2)
                        dismiss()
                    }
                }
            }
        }
        .presentationDetents([.medium])
    }
}
