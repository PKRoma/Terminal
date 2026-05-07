/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- Profiles.h

Abstract:
- The Profiles landing page in the Settings UI. Replaces the previous per-profile
  navigation tree with a single "Profiles" nav item that opens this page. The page
  hosts entry points to the Defaults profile, the Color schemes page, the Add Profile
  flow, and the list of individual profiles.

--*/

#pragma once

#include "Profiles.g.h"
#include "ProfilesPageViewModel.h"
#include "Utils.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct Profiles : public HasScrollViewer<Profiles>, ProfilesT<Profiles>
    {
    public:
        Profiles();

        void OnNavigatedTo(const winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs& e);

        void Defaults_Click(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& args);
        void ColorSchemes_Click(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& args);
        void AddProfile_Click(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& args);
        void Profile_Click(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& args);

        til::property_changed_event PropertyChanged;
        WINRT_PROPERTY(Editor::ProfilesPageViewModel, ViewModel, nullptr);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(Profiles);
}
