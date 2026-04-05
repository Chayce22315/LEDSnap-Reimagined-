import SwiftUI

enum DrawingTool: String, CaseIterable {
    case pen     = "Pen"
    case eraser  = "Eraser"
    case bucket  = "Bucket"
}

struct CanvasView: View {
    var project: BannerProject?

    @StateObject private var viewModel = CanvasViewModel()
    @State private var showPreview = false
    @State private var savedToast = false

    var body: some View {
        VStack(spacing: 0) {
            ToolbarView(
                selectedTool: $viewModel.selectedTool,
                selectedColor: $viewModel.selectedColor,
                ecoMode: $viewModel.ecoMode,
                onSave: { save() },
                onPreview: { showPreview = true }
            )

            GeometryReader { geo in
                let cellSize = min(
                    geo.size.width  / CGFloat(viewModel.engine.gridWidth),
                    geo.size.height / CGFloat(viewModel.engine.gridHeight)
                )

                Canvas { context, _ in
                    guard let cgImage = viewModel.renderedImage else { return }
                    let rect = CGRect(
                        x: 0, y: 0,
                        width:  cellSize * CGFloat(viewModel.engine.gridWidth),
                        height: cellSize * CGFloat(viewModel.engine.gridHeight)
                    )
                    context.draw(Image(decorative: cgImage, scale: 1), in: rect)
                }
                .gesture(
                    DragGesture(minimumDistance: 0)
                        .onChanged { value in
                            let gx = UInt16(max(0, value.location.x / cellSize))
                            let gy = UInt16(max(0, value.location.y / cellSize))
                            viewModel.applyTool(at: gx, y: gy)
                        }
                )
                .drawingGroup()
            }
        }
        .overlay(alignment: .bottom) {
            if savedToast {
                Text("Saved!")
                    .font(.caption.bold())
                    .padding(.horizontal, 16)
                    .padding(.vertical, 8)
                    .background(.green, in: Capsule())
                    .foregroundColor(.white)
                    .transition(.move(edge: .bottom).combined(with: .opacity))
                    .padding(.bottom, 20)
            }
        }
        .animation(.easeInOut, value: savedToast)
        .navigationTitle(project?.name ?? "Canvas")
        .navigationBarTitleDisplayMode(.inline)
        .fullScreenCover(isPresented: $showPreview) {
            BannerView(source: .livePixels(viewModel.engine.snapshotMidground(),
                                           width: viewModel.engine.gridWidth,
                                           height: viewModel.engine.gridHeight,
                                           bgMode: viewModel.engine.backgroundMode,
                                           bgA: viewModel.engine.backgroundColorA,
                                           bgB: viewModel.engine.backgroundColorB))
        }
        .onAppear {
            if let project {
                viewModel.loadProject(project)
            }
            viewModel.startRenderLoop()
        }
        .onDisappear { viewModel.stopRenderLoop() }
    }

    private func save() {
        guard let project else { return }
        let mgr = ProjectManager.shared
        let url = mgr.lsnapURL(for: project)
        if viewModel.engine.saveLsnap(to: url) {
            savedToast = true
            DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) { savedToast = false }
        }
    }
}

@MainActor
final class CanvasViewModel: ObservableObject {
    let engine = LEDSnapEngine(width: 32, height: 16)

    @Published var selectedTool: DrawingTool = .pen
    @Published var selectedColor = Color.red
    @Published var ecoMode = false {
        didSet { engine.ecoMode = ecoMode }
    }
    @Published var renderedImage: CGImage?

    private var displayLink: CADisplayLink?

    func loadProject(_ project: BannerProject) {
        let mgr = ProjectManager.shared
        let url = mgr.lsnapURL(for: project)
        if FileManager.default.fileExists(atPath: url.path) {
            engine.loadLsnap(from: url)
        }
    }

    func applyTool(at x: UInt16, y: UInt16) {
        let uiColor = UIColor(selectedColor)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        uiColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        let px = ls_pixel(UInt8(r * 255), UInt8(g * 255), UInt8(b * 255), UInt8(a * 255))

        switch selectedTool {
        case .pen:    engine.pen(x: x, y: y, color: px)
        case .eraser: engine.eraser(x: x, y: y)
        case .bucket: engine.bucket(x: x, y: y, color: px)
        }
    }

    func startRenderLoop() {
        displayLink = CADisplayLink(target: self, selector: #selector(tick))
        displayLink?.add(to: .main, forMode: .common)
    }

    func stopRenderLoop() {
        displayLink?.invalidate()
        displayLink = nil
    }

    @objc private func tick() {
        engine.tick()
        renderedImage = engine.renderImage()
        engine.endFrame()
    }
}
