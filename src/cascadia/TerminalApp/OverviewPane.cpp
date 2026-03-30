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
    OverviewPane::OverviewPane()
    {
        InitializeComponent();
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

        for (uint32_t i = 0; i < tabs.Size(); i++)
        {
            const auto& tab = tabs.GetAt(i);
            auto cell = _BuildPreviewCell(tab, static_cast<int32_t>(i));
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
        // Reparent content back to original parents
        for (auto& entry : _reparentedContent)
        {
            if (entry.content)
            {
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
        if (auto storyboard = Resources().Lookup(winrt::box_value(L"BackgroundFadeIn")).try_as<Storyboard>())
        {
            storyboard.Begin();
        }
        if (auto storyboard = Resources().Lookup(winrt::box_value(L"ContentFadeIn")).try_as<Storyboard>())
        {
            storyboard.Begin();
        }
    }

    void OverviewPane::_PlayExitAnimation(std::function<void()> onComplete)
    {
        auto bgStoryboard = Resources().Lookup(winrt::box_value(L"BackgroundFadeOut")).try_as<Storyboard>();
        auto contentStoryboard = Resources().Lookup(winrt::box_value(L"ContentFadeOut")).try_as<Storyboard>();

        if (contentStoryboard)
        {
            contentStoryboard.Completed([weakThis = get_weak(), onComplete](auto&&, auto&&) {
                if (auto self = weakThis.get())
                {
                    if (onComplete)
                    {
                        onComplete();
                    }
                }
            });
            contentStoryboard.Begin();
        }
        else if (onComplete)
        {
            onComplete();
        }

        if (bgStoryboard)
        {
            bgStoryboard.Begin();
        }
    }

    FrameworkElement OverviewPane::_BuildPreviewCell(const TerminalApp::Tab& tab, int32_t index)
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
        previewBorder.Width(320);
        previewBorder.Height(220);
        previewBorder.CornerRadius({ 6, 6, 6, 6 });
        previewBorder.Background(SolidColorBrush{ winrt::Windows::UI::ColorHelper::FromArgb(255, 30, 30, 30) });

        // Get the tab's content and reparent it into a Viewbox
        auto tabContent = tab.Content();
        if (tabContent)
        {
            // Track the original parent so we can restore later
            auto originalParent = VisualTreeHelper::GetParent(tabContent).try_as<Panel>();

            // XAML single-parent rule: remove from current parent first
            _DetachContent(tabContent);

            Viewbox viewbox;
            viewbox.Stretch(Stretch::Uniform);
            viewbox.Child(tabContent);

            previewBorder.Child(viewbox);

            _reparentedContent.push_back({ tabContent, originalParent });
        }

        cellStack.Children().Append(previewBorder);

        // Tab title text
        TextBlock titleBlock;
        titleBlock.Text(tab.Title());
        titleBlock.FontSize(14);
        titleBlock.Foreground(SolidColorBrush{ winrt::Windows::UI::Colors::White() });
        titleBlock.HorizontalAlignment(HorizontalAlignment::Center);
        titleBlock.TextTrimming(TextTrimming::CharacterEllipsis);
        titleBlock.MaxWidth(320);
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
