/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- MatchProfilesEntry.h

Abstract:
- An entry in the "new tab" dropdown menu that represents a collection
    of profiles that match a specified name, source, or command line.

Author(s):
- Floris Westerman - November 2022

--*/
#pragma once

#include "ProfileCollectionEntry.h"
#include "MatchProfilesEntry.g.h"

// This macro defines the getter and setter for a regex property.
// The setter tries to instantiate the regex immediately and caches
// it if successful. If it fails, it sets a boolean flag to track that
// it failed.
#define MATCH_PROFILE_REGEX_PROPERTY(name)                         \
public:                                                            \
    hstring name() const noexcept                                  \
    {                                                              \
        return _##name;                                            \
    }                                                              \
    void name(const hstring& value) noexcept                       \
    {                                                              \
        _##name = value;                                           \
        _validate##name();                                         \
    }                                                              \
                                                                   \
private:                                                           \
    void _validate##name() noexcept                                \
    {                                                              \
        _invalid##name = false;                                    \
        try                                                        \
        {                                                          \
            _##name##Regex = { _##name.cbegin(), _##name.cend() }; \
        }                                                          \
        catch (std::regex_error)                                   \
        {                                                          \
            _invalid##name = true;                                 \
        }                                                          \
    }                                                              \
                                                                   \
    hstring _##name;                                               \
    std::wregex _##name##Regex;                                    \
    bool _invalid##name{ false };

namespace winrt::Microsoft::Terminal::Settings::Model::implementation
{
    struct MatchProfilesEntry : MatchProfilesEntryT<MatchProfilesEntry, ProfileCollectionEntry>
    {
    public:
        MatchProfilesEntry() noexcept;

        Model::NewTabMenuEntry Copy() const override;

        Json::Value ToJson() const override;
        static com_ptr<NewTabMenuEntry> FromJson(const Json::Value& json);

        bool ValidateRegexes() const;
        bool MatchesProfile(const Model::Profile& profile);

        MATCH_PROFILE_REGEX_PROPERTY(Name);
        MATCH_PROFILE_REGEX_PROPERTY(Commandline);
        MATCH_PROFILE_REGEX_PROPERTY(Source);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Model::factory_implementation
{
    BASIC_FACTORY(MatchProfilesEntry);
}
