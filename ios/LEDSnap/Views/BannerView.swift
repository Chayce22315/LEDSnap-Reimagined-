import SwiftUI

enum BannerSource {
    case demo
    case livePixels(_ pixels: [LSPixel], width: UInt16, height: UInt16,
                    bgMode: LSBackgroundMode, bgA: LSPixel, bgB: LSPixel)
    case file(URL, width: UInt16, height: UInt16)
}

struct BannerView: View {
    var source: BannerSource = .demo

    @Environment(\.dismiss) private var dismiss
    @StateObject private var viewModel = BannerViewModel()

    var body: some View {
        GeometryReader { geo in
            if let image = viewModel.renderedImage {
                Image(decorative: image, scale: 1)
                    .resizable()
                    .interpolation(.none)
                    .aspectRatio(contentMode: .fill)
                    .frame(width: geo.size.width, height: geo.size.height)
                    .clipped()
            } else {
                Color.black
            }
        }
        .ignoresSafeArea()
        .statusBarHidden()
        .onTapGesture(count: 2) { dismiss() }
        .onAppear {
            UIApplication.shared.isIdleTimerDisabled = true
            viewModel.start(with: source)
        }
        .onDisappear {
            UIApplication.shared.isIdleTimerDisabled = false
            viewModel.stop()
        }
    }
}

@MainActor
final class BannerViewModel: ObservableObject {
    private var engine: LEDSnapEngine?
    @Published var renderedImage: CGImage?

    private var displayLink: CADisplayLink?
    private var textElem = LSTextElement()
    private var useScrollingText = false
    private var bannerText = "LEDSNAP!"

    func start(with source: BannerSource) {
        switch source {
        case .demo:
            setupDemo()

        case let .livePixels(pixels, w, h, bgMode, bgA, bgB):
            let eng = LEDSnapEngine(width: w, height: h, ecoMode: true)
            eng.setBackground(mode: bgMode, colorA: bgA, colorB: bgB)
            eng.loadMidground(from: pixels)
            engine = eng
            useScrollingText = false

        case let .file(url, w, h):
            let eng = LEDSnapEngine(width: w, height: h, ecoMode: true)
            eng.setBackground(mode: LS_BG_SOLID, colorA: ls_pixel(0, 0, 0, 255), colorB: ls_pixel(0, 0, 0, 255))
            eng.loadLsnap(from: url)
            engine = eng
            useScrollingText = false
        }

        displayLink = CADisplayLink(target: self, selector: #selector(tick))
        displayLink?.add(to: .main, forMode: .common)
    }

    func stop() {
        displayLink?.invalidate()
        displayLink = nil
        engine = nil
    }

    private func setupDemo() {
        let eng = LEDSnapEngine(width: 64, height: 16, ecoMode: true)
        eng.setBackground(
            mode: LS_BG_RAINBOW,
            colorA: ls_pixel(0, 0, 0, 255),
            colorB: ls_pixel(0, 0, 0, 255)
        )
        engine = eng
        useScrollingText = true
        bannerText = "LEDSNAP!"

        bannerText.withCString { cstr in
            ls_text_init(&textElem, cstr, -20.0,
                         ls_pixel(255, 255, 255, 255), 4)
        }
        textElem.x_offset = Float(eng.gridWidth)
    }

    @objc private func tick() {
        guard let engine else { return }
        engine.tick()
        let dt = engine.deltaTime

        if useScrollingText {
            ls_text_update(&textElem, dt, engine.gridWidth)
            ls_grid_clear(engine.foregroundGrid)
            bannerText.withCString { cstr in
                textElem.text = cstr
                ls_text_render(&textElem, engine.foregroundGrid)
            }
        }

        renderedImage = engine.renderImage()
        engine.endFrame()
    }
}
