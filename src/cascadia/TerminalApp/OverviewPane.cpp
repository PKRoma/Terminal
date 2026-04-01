// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "OverviewPane.h"

#include "OverviewPane.g.cpp"

using namespace winrt;
using namespace winrt::TerminalApp;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Input;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Media::Animation;
using namespace winrt::Windows::System;

namespace winrt::TerminalApp::implementation
{
    static constexpr double PreviewCellWidth = 320.0;
    static constexpr double PreviewCellHeight = 220.0;
    static constexpr std::chrono::milliseconds EnterAnimDuration{ 400 };
    static constexpr std::chrono::milliseconds ExitAnimDuration{ 100 };

    OverviewPane::OverviewPane()
    {
        InitializeComponent();

        // Listen for layout passes so we can start the zoom-in animation
        // once the WrapGrid has measured and positioned its children.
        PreviewGrid().LayoutUpdated([weakThis = get_weak()](auto&&, auto&&) {
            if (auto self = weakThis.get())
            {
                if (!self->_pendingEnterAnimation)
                {
                    return;
                }

                // Check that at least the first cell is laid out
                auto items = self->PreviewGrid().Items();
                if (items.Size() == 0)
                {
                    return;
                }
                auto first = items.GetAt(0).try_as<FrameworkElement>();
                if (!first || first.ActualWidth() <= 0)
                {
                    return;
                }

                self->_pendingEnterAnimation = false;
                self->_StartEnterZoomAnimation();
            }
        });
    }

    OverviewPane::~OverviewPane()
    {
        ClearTabContent();
    }

    void OverviewPane::UpdateTabContent(Windows::Foundation::Collections::IVector<TerminalApp::Tab> tabs, int32_t focusedIndex)
    {
        // Clear any previous state
        ClearTabContent();

        if (!tabs || tabs.Size() == 0)
        {
            return;
        }

        const auto itemsControl = PreviewGrid();

        // Determine a reference size from the currently visible tab's content.
        // Inactive tabs have zero ActualWidth/Height since they're not laid out
        // in the visual tree, but all tabs share the same content area, so the
        // active tab's size is the right reference for all of them.
        double referenceWidth = 0;
        double referenceHeight = 0;
        if (focusedIndex >= 0 && focusedIndex < static_cast<int32_t>(tabs.Size()))
        {
            auto focusedContent = tabs.GetAt(static_cast<uint32_t>(focusedIndex)).Content();
            if (focusedContent)
            {
                referenceWidth = focusedContent.ActualWidth();
                referenceHeight = focusedContent.ActualHeight();
            }
        }

        for (uint32_t i = 0; i < tabs.Size(); i++)
        {
            const auto& tab = tabs.GetAt(i);
            auto cell = _BuildPreviewCell(tab, static_cast<int32_t>(i), referenceWidth, referenceHeight);
            itemsControl.Items().Append(cell);
        }

        _selectedIndex = focusedIndex;
        _UpdateSelection();
        _PlayEnterAnimation();

        // Focus self for keyboard input
        Focus(FocusState::Programmatic);
    }

    void OverviewPane::ClearTabContent()
    {
        _pendingEnterAnimation = false;

        // Stop any running exit animation
        if (_exitContentStoryboard)
        {
            _exitContentStoryboard.Completed(_exitAnimationToken);
            _exitAnimationToken = {};
            _exitContentStoryboard.Stop();
            _exitContentStoryboard = nullptr;
        }

        // Reset the zoom transform to identity
        auto transform = ContentTransform();
        transform.ScaleX(1.0);
        transform.ScaleY(1.0);
        transform.TranslateX(0.0);
        transform.TranslateY(0.0);
        ContentWrapper().Opacity(1.0);

        // Reparent content back to original parents
        for (auto& entry : _reparentedContent)
        {
            if (entry.content)
            {
                // Restore original Width/Height (NaN = auto-sizing)
                entry.content.Width(entry.originalWidth);
                entry.content.Height(entry.originalHeight);

                // Restore original RenderTransform and origin
                entry.content.RenderTransform(entry.originalRenderTransform);
                entry.content.RenderTransformOrigin(entry.originalRenderTransformOrigin);

                _DetachContent(entry.content);

                // Put it back where it came from
                if (entry.originalParent)
                {
                    entry.originalParent.Children().Append(entry.content);
                }
            }
        }
        _reparentedContent.clear();

        const auto itemsControl = PreviewGrid();
        itemsControl.Items().Clear();
    }

    int32_t OverviewPane::SelectedIndex() const
    {
        return _selectedIndex;
    }

    void OverviewPane::SelectedIndex(int32_t value)
    {
        if (_selectedIndex != value)
        {
            _selectedIndex = value;
            _UpdateSelection();
        }
    }

    void OverviewPane::_OnPreviewKeyDown(const IInspectable& /*sender*/, const KeyRoutedEventArgs& e)
    {
        if (e.OriginalKey() != VirtualKey::Tab)
        {
            return;
        }

        const auto items = PreviewGrid().Items();
        const auto itemCount = static_cast<int32_t>(items.Size());
        if (itemCount == 0)
        {
            return;
        }

        const auto shiftPressed = (Windows::UI::Core::CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift) & Windows::UI::Core::CoreVirtualKeyStates::Down) == Windows::UI::Core::CoreVirtualKeyStates::Down;

        if (shiftPressed)
        {
            if (_selectedIndex > 0)
            {
                _selectedIndex--;
                _UpdateSelection();
            }
        }
        else
        {
            if (_selectedIndex < itemCount - 1)
            {
                _selectedIndex++;
                _UpdateSelection();
            }
        }

        e.Handled(true);
    }

    void OverviewPane::_OnKeyDown(const IInspectable& /*sender*/, const KeyRoutedEventArgs& e)
    {
        const auto items = PreviewGrid().Items();
        const auto itemCount = static_cast<int32_t>(items.Size());
        if (itemCount == 0)
        {
            return;
        }

        auto handled = true;
        switch (e.OriginalKey())
        {
        case VirtualKey::Left:
            if (_selectedIndex > 0)
            {
                _selectedIndex--;
                _UpdateSelection();
            }
            break;
        case VirtualKey::Right:
            if (_selectedIndex < itemCount - 1)
            {
                _selectedIndex++;
                _UpdateSelection();
            }
            break;
        case VirtualKey::Up:
            if (_selectedIndex - _columnCount >= 0)
            {
                _selectedIndex -= _columnCount;
                _UpdateSelection();
            }
            break;
        case VirtualKey::Down:
            if (_selectedIndex + _columnCount < itemCount)
            {
                _selectedIndex += _columnCount;
                _UpdateSelection();
            }
            break;
        case VirtualKey::Enter:
            _OnItemClicked(_selectedIndex);
            break;
        case VirtualKey::Escape:
            _PlayExitAnimation([weakThis = get_weak()]() {
                if (auto self = weakThis.get())
                {
                    self->Dismissed.raise(*self, nullptr);
                }
            });
            break;
        default:
            handled = false;
            break;
        }

        e.Handled(handled);
    }

    void OverviewPane::_OnItemClicked(int32_t index)
    {
        _PlayExitAnimation([weakThis = get_weak(), index]() {
            if (auto self = weakThis.get())
            {
                self->TabSelected.raise(*self, winrt::Windows::Foundation::IReference<int32_t>{ index });
            }
        });
    }

    void OverviewPane::_UpdateSelection()
    {
        const auto items = PreviewGrid().Items();
        const auto itemCount = static_cast<int32_t>(items.Size());

        // Clamp selection
        _selectedIndex = std::clamp(_selectedIndex, 0, std::max(0, itemCount - 1));

        for (int32_t i = 0; i < itemCount; i++)
        {
            if (auto cellElement = items.GetAt(i).try_as<FrameworkElement>())
            {
                if (auto border = cellElement.try_as<Border>())
                {
                    if (i == _selectedIndex)
                    {
                        // Accent-colored border for selected item
                        const auto accentBrush = Application::Current()
                                                     .Resources()
                                                     .Lookup(winrt::box_value(L"SystemAccentColor"))
                                                     .as<winrt::Windows::UI::Color>();
                        border.BorderBrush(SolidColorBrush{ accentBrush });
                        border.BorderThickness(ThicknessHelper::FromUniformLength(2));

                        // Scroll into view if needed
                        border.StartBringIntoView();
                    }
                    else
                    {
                        border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Colors::Transparent() });
                        border.BorderThickness(ThicknessHelper::FromUniformLength(2));
                    }
                }
            }
        }
    }

    void OverviewPane::_PlayEnterAnimation()
    {
        // Hide the content wrapper until the LayoutUpdated callback fires
        // and we can read cell positions to set up the zoom transform.
        ContentWrapper().Opacity(0);
        _pendingEnterAnimation = true;
    }

    void OverviewPane::_StartEnterZoomAnimation()
    {
        // Start the background fade-in together with the zoom so both
        // animations are visible at the same moment (avoids opacity flash).
        if (auto bgSb = Resources().Lookup(winrt::box_value(L"BackgroundFadeIn")).try_as<Storyboard>())
        {
            bgSb.Begin();
        }

        auto wrapper = ContentWrapper();
        auto transform = ContentTransform();

        auto zoomParams = _GetZoomParamsForCell(_selectedIndex);
        if (!zoomParams)
        {
            wrapper.Opacity(1.0);
            return;
        }
        auto [scale, tx, ty] = *zoomParams;

        // Set the initial transform so the focused cell fills the viewport
        transform.ScaleX(scale);
        transform.ScaleY(scale);
        transform.TranslateX(tx);
        transform.TranslateY(ty);
        wrapper.Opacity(1.0);

        // Animate from the zoomed-in state to identity (zoom out to grid)
        Storyboard storyboard;
        const auto duration = DurationHelper::FromTimeSpan(EnterAnimDuration);
        CubicEase easing;
        easing.EasingMode(EasingMode::EaseOut);

        _AddDoubleAnimation(storyboard, transform, L"ScaleX", scale, 1.0, duration, easing);
        _AddDoubleAnimation(storyboard, transform, L"ScaleY", scale, 1.0, duration, easing);
        _AddDoubleAnimation(storyboard, transform, L"TranslateX", tx, 0.0, duration, easing);
        _AddDoubleAnimation(storyboard, transform, L"TranslateY", ty, 0.0, duration, easing);

        storyboard.Begin();
    }

    void OverviewPane::_PlayExitAnimation(std::function<void()> onComplete)
    {
        // Fade out the background overlay
        if (auto bgSb = Resources().Lookup(winrt::box_value(L"BackgroundFadeOut")).try_as<Storyboard>())
        {
            bgSb.Begin();
        }

        auto zoomParams = _GetZoomParamsForCell(_selectedIndex);
        if (!zoomParams)
        {
            if (onComplete)
            {
                onComplete();
            }
            return;
        }
        auto [scale, tx, ty] = *zoomParams;

        // Animate from the current grid view into the selected cell
        auto transform = ContentTransform();
        Storyboard storyboard;
        const auto duration = DurationHelper::FromTimeSpan(ExitAnimDuration);
        CubicEase easing;
        easing.EasingMode(EasingMode::EaseIn);

        _AddDoubleAnimation(storyboard, transform, L"ScaleX", 1.0, scale, duration, easing);
        _AddDoubleAnimation(storyboard, transform, L"ScaleY", 1.0, scale, duration, easing);
        _AddDoubleAnimation(storyboard, transform, L"TranslateX", 0.0, tx, duration, easing);
        _AddDoubleAnimation(storyboard, transform, L"TranslateY", 0.0, ty, duration, easing);

        // Revoke any previously registered Completed handler
        if (_exitContentStoryboard)
        {
            _exitContentStoryboard.Completed(_exitAnimationToken);
        }
        _exitContentStoryboard = storyboard;
        _exitAnimationToken = storyboard.Completed([weakThis = get_weak(), onComplete](auto&&, auto&&) {
            if (auto self = weakThis.get())
            {
                if (onComplete)
                {
                    onComplete();
                }
            }
        });

        storyboard.Begin();
    }

    std::optional<std::tuple<double, double, double>> OverviewPane::_GetZoomParamsForCell(int32_t index)
    {
        auto wrapper = ContentWrapper();
        const auto vpW = wrapper.ActualWidth();
        const auto vpH = wrapper.ActualHeight();

        if (vpW <= 0 || vpH <= 0)
        {
            return std::nullopt;
        }

        auto items = PreviewGrid().Items();
        if (index < 0 || index >= static_cast<int32_t>(items.Size()))
        {
            return std::nullopt;
        }

        auto cell = items.GetAt(static_cast<uint32_t>(index)).try_as<FrameworkElement>();
        if (!cell || cell.ActualWidth() <= 0 || cell.ActualHeight() <= 0)
        {
            return std::nullopt;
        }

        const auto cellW = cell.ActualWidth();
        const auto cellH = cell.ActualHeight();

        // Cell center relative to ContentWrapper (accounts for scroll offset)
        auto cellTransform = cell.TransformToVisual(wrapper);
        auto topLeft = cellTransform.TransformPoint({ 0.0f, 0.0f });
        const auto cellCX = static_cast<double>(topLeft.X) + cellW / 2.0;
        const auto cellCY = static_cast<double>(topLeft.Y) + cellH / 2.0;

        // Scale so the cell fits the viewport
        const auto scale = std::min(vpW / cellW, vpH / cellH);

        // With RenderTransformOrigin={0.5,0.5} the scale origin is the
        // center of ContentWrapper. Translate so the cell center lands
        // at the viewport center after scaling.
        const auto translateX = (vpW / 2.0 - cellCX) * scale;
        const auto translateY = (vpH / 2.0 - cellCY) * scale;

        return std::tuple{ scale, translateX, translateY };
    }

    FrameworkElement OverviewPane::_BuildPreviewCell(const TerminalApp::Tab& tab, int32_t index, double referenceWidth, double referenceHeight)
    {
        // Outer border — serves as the selection indicator
        Border outerBorder;
        outerBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Colors::Transparent() });
        outerBorder.BorderThickness(ThicknessHelper::FromUniformLength(2));
        outerBorder.CornerRadius({ 8, 8, 8, 8 });
        outerBorder.Padding(ThicknessHelper::FromUniformLength(4));
        outerBorder.Margin(ThicknessHelper::FromUniformLength(6));

        // Vertical stack: preview box + title
        StackPanel cellStack;
        cellStack.Orientation(Orientation::Vertical);
        cellStack.HorizontalAlignment(HorizontalAlignment::Center);

        // Preview container with a dark background
        Border previewBorder;
        previewBorder.Width(PreviewCellWidth);
        previewBorder.Height(PreviewCellHeight);
        previewBorder.CornerRadius({ 6, 6, 6, 6 });
        auto bgBrush = Application::Current().Resources().TryLookup(winrt::box_value(L"SystemControlBackgroundChromeMediumBrush"));
        if (bgBrush)
        {
            previewBorder.Background(bgBrush.as<Brush>());
        }
        else
        {
            previewBorder.Background(SolidColorBrush{ winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30) });
        }

        // Get the tab's content and reparent it with a ScaleTransform
        auto tabContent = tab.Content();
        if (tabContent)
        {
            // Save the Width/Height *property* values (likely NaN for auto-sized
            // elements). These differ from ActualWidth/Height (the rendered size).
            // We need the property values to restore auto-sizing on exit.
            const auto origWidthProp = tabContent.Width();
            const auto origHeightProp = tabContent.Height();
            const auto origRenderTransform = tabContent.RenderTransform();
            const auto origRenderTransformOrigin = tabContent.RenderTransformOrigin();

            // Use ActualWidth/Height if the content is currently laid out (active tab).
            // Inactive tabs aren't in the visual tree and report zero — fall back
            // to the reference size from the active tab's content area.
            auto layoutWidth = tabContent.ActualWidth();
            auto layoutHeight = tabContent.ActualHeight();
            if (layoutWidth <= 0 || layoutHeight <= 0)
            {
                layoutWidth = referenceWidth;
                layoutHeight = referenceHeight;
            }

            auto originalParent = VisualTreeHelper::GetParent(tabContent).try_as<Panel>();

            // XAML single-parent rule: remove from current parent first
            _DetachContent(tabContent);

            // Lock the content to the layout size — this prevents
            // TermControl from seeing a resize and reflowing its buffer
            if (layoutWidth > 0 && layoutHeight > 0)
            {
                tabContent.Width(layoutWidth);
                tabContent.Height(layoutHeight);

                // Calculate uniform scale to fit in preview
                const double previewWidth = PreviewCellWidth;
                const double previewHeight = PreviewCellHeight;
                const double scale = std::min(previewWidth / layoutWidth, previewHeight / layoutHeight);

                // RenderTransform is applied AFTER layout — the content still
                // thinks it's at its original size
                ScaleTransform scaleTransform;
                scaleTransform.ScaleX(scale);
                scaleTransform.ScaleY(scale);
                tabContent.RenderTransform(scaleTransform);
                tabContent.RenderTransformOrigin({ 0.0f, 0.0f });

                // Use a Canvas so the content is not constrained by the preview
                // container's layout. Canvas gives children infinite measure
                // space and arranges at desired size.
                Canvas canvas;
                canvas.Width(previewWidth);
                canvas.Height(previewHeight);

                // Clip to preview bounds so the scaled content doesn't overflow
                RectangleGeometry clipGeometry;
                clipGeometry.Rect({ 0, 0, static_cast<float>(previewWidth), static_cast<float>(previewHeight) });
                canvas.Clip(clipGeometry);

                canvas.Children().Append(tabContent);

                // Layer the canvas behind a transparent overlay so
                // pointer events never reach the TermControl content.
                Grid previewGrid;
                previewGrid.Children().Append(canvas);

                Border inputOverlay;
                inputOverlay.Background(SolidColorBrush{ winrt::Windows::UI::Colors::Transparent() });
                previewGrid.Children().Append(inputOverlay);

                previewBorder.Child(previewGrid);
            }

            _reparentedContent.push_back({ tabContent, originalParent, origWidthProp, origHeightProp, origRenderTransform, origRenderTransformOrigin });
        }

        cellStack.Children().Append(previewBorder);

        // Tab title text
        TextBlock titleBlock;
        titleBlock.Text(tab.Title());
        titleBlock.FontSize(14);
        titleBlock.Foreground(SolidColorBrush{ winrt::Windows::UI::Colors::White() });
        titleBlock.HorizontalAlignment(HorizontalAlignment::Center);
        titleBlock.TextTrimming(TextTrimming::CharacterEllipsis);
        titleBlock.MaxWidth(PreviewCellWidth);
        titleBlock.Margin({ 0, 6, 0, 0 });

        cellStack.Children().Append(titleBlock);

        outerBorder.Child(cellStack);

        // Click handler
        outerBorder.PointerPressed([weakThis = get_weak(), index](auto&&, auto&&) {
            if (auto self = weakThis.get())
            {
                self->_selectedIndex = index;
                self->_UpdateSelection();
                self->_OnItemClicked(index);
            }
        });

        // Hover effect
        outerBorder.PointerEntered([weakThis = get_weak(), index](auto&&, auto&&) {
            if (auto self = weakThis.get())
            {
                self->_selectedIndex = index;
                self->_UpdateSelection();
            }
        });

        return outerBorder;
    }

    void OverviewPane::_AddDoubleAnimation(
        const Storyboard& storyboard,
        const CompositeTransform& target,
        const hstring& property,
        double from,
        double to,
        const Duration& duration,
        const EasingFunctionBase& easing)
    {
        DoubleAnimation anim;
        anim.From(from);
        anim.To(to);
        anim.Duration(duration);
        anim.EasingFunction(easing);
        Storyboard::SetTarget(anim, target);
        Storyboard::SetTargetProperty(anim, property);
        storyboard.Children().Append(anim);
    }

    void OverviewPane::_DetachContent(const FrameworkElement& content)
    {
        // Try removing from various XAML container types
        if (auto parent = VisualTreeHelper::GetParent(content))
        {
            if (auto panel = parent.try_as<Panel>())
            {
                uint32_t idx;
                if (panel.Children().IndexOf(content, idx))
                {
                    panel.Children().RemoveAt(idx);
                }
            }
            else if (auto border = parent.try_as<Border>())
            {
                border.Child(nullptr);
            }
            else if (auto contentControl = parent.try_as<ContentControl>())
            {
                contentControl.Content(nullptr);
            }
            else if (auto viewbox = parent.try_as<Viewbox>())
            {
                viewbox.Child(nullptr);
            }
        }
    }
}
