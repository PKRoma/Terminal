// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "ProfilesPageViewModel.h"
#include "ProfilesPageViewModel.g.cpp"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    ProfilesPageViewModel::ProfilesPageViewModel(const IObservableVector<Editor::ProfileViewModel>& profileVMs)
    {
        _setProfiles(profileVMs);
    }

    void ProfilesPageViewModel::UpdateProfileVMs(const IObservableVector<Editor::ProfileViewModel>& profileVMs)
    {
        Profiles(profileVMs);
    }

    void ProfilesPageViewModel::RequestOpenDefaults()
    {
        OpenDefaultsRequested.raise(*this, nullptr);
    }

    void ProfilesPageViewModel::RequestOpenColorSchemes()
    {
        OpenColorSchemesRequested.raise(*this, nullptr);
    }

    void ProfilesPageViewModel::RequestAddProfile()
    {
        AddProfileRequested.raise(*this, nullptr);
    }

    void ProfilesPageViewModel::RequestOpenProfile(const Editor::ProfileViewModel& profile)
    {
        OpenProfileRequested.raise(*this, profile);
    }
}
