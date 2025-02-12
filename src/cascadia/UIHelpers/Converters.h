// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "Converters.g.h"

namespace winrt::Microsoft::Terminal::UI::implementation
{
    struct Converters
    {
        // Booleans
        static bool InvertBoolean(bool value);
        static winrt::Windows::UI::Xaml::Visibility InvertedBooleanToVisibility(bool value);

        // Numbers
        static double PercentageToPercentageValue(double value);
        static double PercentageValueToPercentage(double value);
        static winrt::hstring PercentageToPercentageString(double value);

        // Strings
        static bool StringsAreNotEqual(const winrt::hstring& expected, const winrt::hstring& actual);
        static bool StringNotEmpty(const winrt::hstring& value);
        static winrt::Windows::UI::Xaml::Visibility StringNotEmptyToVisibility(const winrt::hstring& value);
        static winrt::hstring StringOrEmptyIfPlaceholder(const winrt::hstring& placeholder, const winrt::hstring& value);

        // Misc
        static winrt::Windows::UI::Text::FontWeight DoubleToFontWeight(double value);
        static winrt::Windows::UI::Xaml::Media::SolidColorBrush ColorToBrush(winrt::Windows::UI::Color color);
        static double FontWeightToDouble(winrt::Windows::UI::Text::FontWeight fontWeight);
        static double MaxValueFromPaddingString(const winrt::hstring& paddingString);

        // Unboxing
        static winrt::hstring UnboxString(const Windows::Foundation::IInspectable& value);
        static uint32_t UnboxUInt32(const Windows::Foundation::IInspectable& value);
        static uint32_t UnboxUInt32Optional(const Windows::Foundation::IInspectable& value);
        static float UnboxFloat(const Windows::Foundation::IInspectable& value);
        static bool UnboxBool(const Windows::Foundation::IInspectable& value);
        static winrt::Windows::Foundation::IReference<bool> UnboxBoolOptional(const Windows::Foundation::IInspectable& value);
    };
}

namespace winrt::Microsoft::Terminal::UI::factory_implementation
{
    struct Converters : ConvertersT<Converters, implementation::Converters>
    {
    };
}
