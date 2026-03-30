// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "OverviewPane.g.h"

namespace winrt::TerminalApp::implementation
{
    struct OverviewPane : OverviewPaneT<OverviewPane>
    {
        OverviewPane();

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
        void _OnItemClicked(int32_t index);
        void _UpdateSelection();
        void _PlayEnterAnimation();
        void _PlayExitAnimation(std::function<void()> onComplete = nullptr);
        Windows::UI::Xaml::FrameworkElement _BuildPreviewCell(const TerminalApp::Tab& tab, int32_t index);
        void _DetachContent(const Windows::UI::Xaml::FrameworkElement& content);

        int32_t _selectedIndex{ 0 };
        int32_t _columnCount{ 3 };

        struct ReparentedEntry
        {
            Windows::UI::Xaml::FrameworkElement content{ nullptr };
            Windows::UI::Xaml::Controls::Panel originalParent{ nullptr };
        };
        std::vector<ReparentedEntry> _reparentedContent;
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(OverviewPane);
}
