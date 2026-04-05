import SwiftUI

struct ToolbarView: View {
    @Binding var selectedTool: DrawingTool
    @Binding var selectedColor: Color
    @Binding var ecoMode: Bool
    var onSave: (() -> Void)?
    var onPreview: (() -> Void)?

    var body: some View {
        VStack(spacing: 0) {
            HStack(spacing: 12) {
                ForEach(DrawingTool.allCases, id: \.self) { tool in
                    Button {
                        selectedTool = tool
                    } label: {
                        Text(tool.rawValue)
                            .font(.caption.bold())
                            .padding(.horizontal, 10)
                            .padding(.vertical, 6)
                            .background(selectedTool == tool ? Color.accentColor : Color.gray.opacity(0.2))
                            .foregroundColor(selectedTool == tool ? .white : .primary)
                            .cornerRadius(8)
                    }
                }

                Spacer()

                ColorPicker("", selection: $selectedColor)
                    .labelsHidden()
                    .frame(width: 30, height: 30)
            }
            .padding(.horizontal)
            .padding(.vertical, 6)

            HStack(spacing: 12) {
                if let onSave {
                    Button(action: onSave) {
                        Label("Save", systemImage: "square.and.arrow.down")
                            .font(.caption.bold())
                            .padding(.horizontal, 10)
                            .padding(.vertical, 6)
                            .background(Color.green.opacity(0.15))
                            .foregroundColor(.green)
                            .cornerRadius(8)
                    }
                }

                if let onPreview {
                    Button(action: onPreview) {
                        Label("Preview", systemImage: "play.fill")
                            .font(.caption.bold())
                            .padding(.horizontal, 10)
                            .padding(.vertical, 6)
                            .background(Color.orange.opacity(0.15))
                            .foregroundColor(.orange)
                            .cornerRadius(8)
                    }
                }

                Spacer()

                Toggle("Eco", isOn: $ecoMode)
                    .toggleStyle(.switch)
                    .fixedSize()
            }
            .padding(.horizontal)
            .padding(.vertical, 6)
        }
        .background(.ultraThinMaterial)
    }
}
