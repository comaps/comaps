import SwiftUI

struct CollapsedSearchField<Fallback: View>: View {
    @Environment(SearchModel.self) private var model: SearchModel?
    @ViewBuilder let fallback: () -> Fallback
    
    
    var body: some View {
        if let model, model.isCollapsed {
            HStack(spacing: 0) {
                Button(action: model.expand) {
                    Label {
                        if model.text.isEmpty {
                            Text("search")
                                .foregroundStyle(.secondary)
                        } else {
                            Text(verbatim: model.text)
                                .foregroundStyle(.primary)
                        }
                    } icon: {
                        Image(systemName: "magnifyingglass")
                            .foregroundStyle(.secondary)
                    }
                    .lineLimit(1)
                    .padding(.leading)
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
                    .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .accessibilityLabel(Text("search"))
                .accessibilityValue(model.text)
                .accessibilityActions {
                    if !model.text.isEmpty {
                        Button(L("search_results")) {
                            model.didSelectStep(.halfScreen)
                        }
                    }
                }
                .accessibilityShowsLargeContentViewer()
                
                Button(action: model.didTapClose) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundStyle(.secondary)
                        .padding(.horizontal)
                        .frame(maxHeight: .infinity)
                        .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .accessibilityLabel(L("close"))
                .accessibilityShowsLargeContentViewer()
            }
            .overlay(alignment: .top) {
                Capsule()
                    .fill(.secondary)
                    .frame(width: 36, height: 5)
                    .padding(.top, 5)
                    .accessibilityHidden(true)
            }
            .highPriorityGesture(
                DragGesture(minimumDistance: 10)
                    .onEnded { value in
                        if value.translation.height < -30 || value.predictedEndTranslation.height < -80 {
                            model.didSelectStep(.halfScreen)
                        }
                    }
            )
            .dynamicTypeSize(...DynamicTypeSize.xxxLarge)
        } else {
            fallback()
        }
    }
}
