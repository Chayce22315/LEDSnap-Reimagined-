import Foundation
import UIKit

final class LEDSnapEngine {
    private var compositor = LSCompositor()
    private var limiter = LSFrameLimiter()
    private var animation = LSAnimation()

    let gridWidth: UInt16
    let gridHeight: UInt16

    init(width: UInt16 = 32, height: UInt16 = 16, ecoMode: Bool = false) {
        gridWidth  = width
        gridHeight = height
        ls_compositor_init(&compositor, width, height)
        ls_limiter_init(&limiter, ecoMode ? 1 : 0)
        ls_animation_init(&animation, width, height, 256)
    }

    deinit {
        ls_compositor_destroy(&compositor)
        ls_animation_destroy(&animation)
    }

    // MARK: - Background

    var backgroundMode: LSBackgroundMode {
        get { compositor.bg_mode }
    }

    var backgroundColorA: LSPixel {
        get { compositor.bg_color_a }
    }

    var backgroundColorB: LSPixel {
        get { compositor.bg_color_b }
    }

    func setBackground(mode: LSBackgroundMode, colorA: LSPixel, colorB: LSPixel) {
        ls_compositor_set_background(&compositor, mode, colorA, colorB)
    }

    // MARK: - Drawing tools

    func pen(x: UInt16, y: UInt16, color: LSPixel) {
        ls_tool_pen(&compositor.midground, x, y, color)
    }

    func eraser(x: UInt16, y: UInt16) {
        ls_tool_eraser(&compositor.midground, x, y)
    }

    func bucket(x: UInt16, y: UInt16, color: LSPixel) {
        ls_tool_bucket(&compositor.midground, x, y, color)
    }

    // MARK: - Frame tick

    func tick() {
        ls_limiter_begin(&limiter)
        ls_compositor_flatten(&compositor, Float(limiter.delta_time))
    }

    func endFrame() {
        ls_limiter_end(&limiter)
    }

    var deltaTime: Float {
        Float(limiter.delta_time)
    }

    // MARK: - Eco mode

    var ecoMode: Bool {
        get { limiter.eco_mode != 0 }
        set {
            if newValue != (limiter.eco_mode != 0) {
                ls_limiter_toggle_eco(&limiter)
            }
        }
    }

    // MARK: - Animation

    func addAnimationFrame() {
        ls_animation_add_frame(&animation, &compositor.midground)
    }

    var animationFrameCount: Int {
        Int(animation.frame_count)
    }

    // MARK: - Snapshot: capture current midground pixels into a standalone grid

    func snapshotMidground() -> [LSPixel] {
        let count = Int(gridWidth) * Int(gridHeight)
        guard let px = compositor.midground.pixels else { return [] }
        return Array(UnsafeBufferPointer(start: px, count: count))
    }

    func loadMidground(from pixels: [LSPixel]) {
        let count = min(pixels.count, Int(gridWidth) * Int(gridHeight))
        guard let dst = compositor.midground.pixels else { return }
        pixels.withUnsafeBufferPointer { src in
            dst.update(from: src.baseAddress!, count: count)
        }
    }

    // MARK: - .lsnap save / load

    func saveLsnap(to url: URL, fps: UInt16 = 30, mode: LSModeFlag = LS_MODE_SOLO) -> Bool {
        ls_animation_add_frame(&animation, &compositor.midground)

        var snap = LSSnapFile()
        ls_snap_init(&snap, gridWidth, gridHeight, fps, mode)

        if ls_snap_build_palette(&snap, &animation) != 0 {
            ls_snap_free(&snap)
            return false
        }

        let total = Int(gridWidth) * Int(gridHeight)
        let capacity = UInt32(total)
        let dirtyBuf = UnsafeMutablePointer<LSDirtyPixel>.allocate(capacity: total)
        defer { dirtyBuf.deallocate() }

        snap.frame_count = UInt32(animation.frame_count)
        snap.frames = UnsafeMutablePointer<LSSnapFrame>.allocate(capacity: Int(animation.frame_count))

        var prevGrid: UnsafePointer<LSGrid>? = nil
        for i in 0..<animation.frame_count {
            ls_animation_set_frame(&animation, i)
            guard let cur = ls_animation_current(&animation) else { continue }

            let dirtyCount = ls_snap_diff_frame(prevGrid, cur, dirtyBuf, capacity, &snap)
            let frame = UnsafeMutablePointer<LSSnapFrame>.allocate(capacity: 1)
            frame.pointee.dirty_count = dirtyCount
            frame.pointee.dirty_pixels = UnsafeMutablePointer<LSDirtyPixel>.allocate(capacity: Int(dirtyCount))
            frame.pointee.dirty_pixels.update(from: dirtyBuf, count: Int(dirtyCount))

            snap.frames[Int(i)] = frame.pointee
            prevGrid = cur
        }

        let result = ls_snap_write(url.path, &snap)

        for i in 0..<Int(snap.frame_count) {
            snap.frames[i].dirty_pixels.deallocate()
        }
        snap.frames.deallocate()
        snap.frames = nil
        snap.frame_count = 0

        return result == 0
    }

    func loadLsnap(from url: URL) -> Bool {
        var snap = LSSnapFile()
        guard ls_snap_read(url.path, &snap) == 0 else { return false }
        defer { ls_snap_free(&snap) }

        ls_grid_clear(&compositor.midground)

        if snap.frame_count > 0 {
            let fr = snap.frames[0]
            for d in 0..<Int(fr.dirty_count) {
                let dp = fr.dirty_pixels[d]
                let pidx = Int(dp.palette_idx)
                guard pidx < snap.palette_size else { continue }
                guard ls_grid_in_bounds(&compositor.midground, Int32(dp.x), Int32(dp.y)) != 0 else { continue }
                var color = LSPixel()
                color.r = snap.palette[pidx].0
                color.g = snap.palette[pidx].1
                color.b = snap.palette[pidx].2
                color.a = 255
                ls_grid_at(&compositor.midground, dp.x, dp.y).pointee = color
            }
        }
        return true
    }

    // MARK: - Output as CGImage

    func renderImage() -> CGImage? {
        let output = ls_compositor_get_output(&compositor)
        guard let pixels = output?.pointee.pixels else { return nil }

        let w = Int(gridWidth)
        let h = Int(gridHeight)
        let bitsPerComponent = 8
        let bytesPerPixel    = 4
        let bytesPerRow      = w * bytesPerPixel
        let totalBytes       = h * bytesPerRow

        var data = Data(count: totalBytes)
        data.withUnsafeMutableBytes { raw in
            guard let dest = raw.baseAddress?.assumingMemoryBound(to: UInt8.self) else { return }
            for i in 0..<(w * h) {
                let px = pixels[i]
                dest[i * 4 + 0] = px.r
                dest[i * 4 + 1] = px.g
                dest[i * 4 + 2] = px.b
                dest[i * 4 + 3] = px.a
            }
        }

        let colorSpace = CGColorSpaceCreateDeviceRGB()
        guard let provider = CGDataProvider(data: data as CFData) else { return nil }

        return CGImage(
            width: w, height: h,
            bitsPerComponent: bitsPerComponent,
            bitsPerPixel: bytesPerPixel * bitsPerComponent,
            bytesPerRow: bytesPerRow,
            space: colorSpace,
            bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue),
            provider: provider,
            decode: nil,
            shouldInterpolate: false,
            intent: .defaultIntent
        )
    }

    // MARK: - Direct access for advanced use

    var midgroundGrid: UnsafeMutablePointer<LSGrid> {
        withUnsafeMutablePointer(to: &compositor.midground) { $0 }
    }

    var foregroundGrid: UnsafeMutablePointer<LSGrid> {
        withUnsafeMutablePointer(to: &compositor.foreground) { $0 }
    }
}
