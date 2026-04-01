// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <limits>
#include <optional>

#include "OverviewPane.g.h"

namespace winrt::TerminalApp::implementation
{
    struct OverviewPane : OverviewPaneT<OverviewPane>
    {
        OverviewPane();
        ~OverviewPane();

        void UpdateTabContent(Windows::Foundation::Collections::IVector<TerminalApp::Tab> tabs, int32_t focusedIndex);
        void ClearTabContent();

        int32_t SelectedIndex() const;
        void SelectedIndex(int32_t value);

        // Events
        til::typed_event<Windows::Foundation::IInspectable, Windows::Foundation::IReference<int32_t>> TabSelected;
        til::typed_event<> Dismissed;

    private:
        friend struct OverviewPaneT<OverviewPane>; // for Xaml to bind events

        void _OnKeyDown(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::Input::KeyRoutedEventArgs& e);
        void _OnPreviewKeyDown(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::Input::KeyRoutedEventArgs& e);
        void _OnItemClicked(int32_t index);
        void _UpdateSelection();
        void _PlayEnterAnimation();
        void _PlayExitAnimation(std::function<void()> onComplete = nullptr);
        void _StartEnterZoomAnimation();
        std::optional<std::tuple<double, double, double>> _GetZoomParamsForCell(int32_t index);
        Windows::UI::Xaml::FrameworkElement _BuildPreviewCell(const TerminalApp::Tab& tab, int32_t index, double referenceWidth, double referenceHeight);
        void _DetachContent(const Windows::UI::Xaml::FrameworkElement& content);
        static void _AddDoubleAnimation(
            const Windows::UI::Xaml::Media::Animation::Storyboard& storyboard,
            const Windows::UI::Xaml::Media::CompositeTransform& target,
            const hstring& property,
            double from,
            double to,
            const Windows::UI::Xaml::Duration& duration,
            const Windows::UI::Xaml::Media::Animation::EasingFunctionBase& easing);

        int32_t _selectedIndex{ 0 };
        int32_t _columnCount{ 3 }; // must match WrapGrid MaximumRowsOrColumns in OverviewPane.xaml
        bool _pendingEnterAnimation{ false };
        winrt::event_token _exitAnimationToken{};
        Windows::UI::Xaml::Media::Animation::Storyboard _exitContentStoryboard{ nullptr };

        struct ReparentedEntry
        {
            Windows::UI::Xaml::FrameworkElement content{ nullptr };
            Windows::UI::Xaml::Controls::Panel originalParent{ nullptr };
            double originalWidth{ std::numeric_limits<double>::quiet_NaN() };
            double originalHeight{ std::numeric_limits<double>::quiet_NaN() };
            Windows::UI::Xaml::Media::Transform originalRenderTransform{ nullptr };
            Windows::Foundation::Point originalRenderTransformOrigin{ 0.0f, 0.0f };
        };
        std::vector<ReparentedEntry> _reparentedContent;
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(OverviewPane);
}
