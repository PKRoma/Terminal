// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "ArgsTemplateSelectors.g.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct ArgsTemplateSelectors : ArgsTemplateSelectorsT<ArgsTemplateSelectors>
    {
        ArgsTemplateSelectors() = default;

        Windows::UI::Xaml::DataTemplate SelectTemplateCore(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::UI::Xaml::DependencyObject&);
        Windows::UI::Xaml::DataTemplate SelectTemplateCore(const winrt::Windows::Foundation::IInspectable&);

        WINRT_PROPERTY(winrt::Windows::UI::Xaml::DataTemplate, UInt32Template);
        WINRT_PROPERTY(winrt::Windows::UI::Xaml::DataTemplate, StringTemplate);
        WINRT_PROPERTY(winrt::Windows::UI::Xaml::DataTemplate, BoolTemplate);
        WINRT_PROPERTY(winrt::Windows::UI::Xaml::DataTemplate, BoolOptionalTemplate);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(ArgsTemplateSelectors);
}
