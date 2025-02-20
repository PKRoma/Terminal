// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "ActionsViewModel.g.h"
#include "KeyBindingViewModel.g.h"
#include "CommandViewModel.g.h"
#include "ArgWrapper.g.h"
#include "ActionArgsViewModel.g.h"
#include "KeyChordViewModel.g.h"
#include "ModifyKeyBindingEventArgs.g.h"
#include "ModifyKeyChordEventArgs.g.h"
#include "Utils.h"
#include "ViewModelHelpers.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct KeyBindingViewModelComparator
    {
        bool operator()(const Editor::KeyBindingViewModel& lhs, const Editor::KeyBindingViewModel& rhs) const
        {
            return lhs.Name() < rhs.Name();
        }
    };

    struct CommandViewModelComparator
    {
        bool operator()(const Editor::CommandViewModel& lhs, const Editor::CommandViewModel& rhs) const
        {
            return lhs.Name() < rhs.Name();
        }
    };

    struct ModifyKeyBindingEventArgs : ModifyKeyBindingEventArgsT<ModifyKeyBindingEventArgs>
    {
    public:
        ModifyKeyBindingEventArgs(const Control::KeyChord& oldKeys, const Control::KeyChord& newKeys, const hstring oldActionName, const hstring newActionName) :
            _OldKeys{ oldKeys },
            _NewKeys{ newKeys },
            _OldActionName{ std::move(oldActionName) },
            _NewActionName{ std::move(newActionName) } {}

        WINRT_PROPERTY(Control::KeyChord, OldKeys, nullptr);
        WINRT_PROPERTY(Control::KeyChord, NewKeys, nullptr);
        WINRT_PROPERTY(hstring, OldActionName);
        WINRT_PROPERTY(hstring, NewActionName);
    };

    struct ModifyKeyChordEventArgs : ModifyKeyChordEventArgsT<ModifyKeyChordEventArgs>
    {
    public:
        ModifyKeyChordEventArgs(const Control::KeyChord& oldKeys, const Control::KeyChord& newKeys) :
            _OldKeys{ oldKeys },
            _NewKeys{ newKeys } {}

        WINRT_PROPERTY(Control::KeyChord, OldKeys, nullptr);
        WINRT_PROPERTY(Control::KeyChord, NewKeys, nullptr);
    };

    struct KeyBindingViewModel : KeyBindingViewModelT<KeyBindingViewModel>, ViewModelHelper<KeyBindingViewModel>
    {
    public:
        KeyBindingViewModel(const Windows::Foundation::Collections::IObservableVector<hstring>& availableActions);
        KeyBindingViewModel(const Control::KeyChord& keys, const hstring& name, const Windows::Foundation::Collections::IObservableVector<hstring>& availableActions);

        hstring Name() const { return _CurrentAction; }
        hstring KeyChordText() const { return _KeyChordText; }

        // UIA Text
        hstring EditButtonName() const noexcept;
        hstring CancelButtonName() const noexcept;
        hstring AcceptButtonName() const noexcept;
        hstring DeleteButtonName() const noexcept;

        void EnterHoverMode() { IsHovered(true); };
        void ExitHoverMode() { IsHovered(false); };
        void ActionGotFocus() { IsContainerFocused(true); };
        void ActionLostFocus() { IsContainerFocused(false); };
        void EditButtonGettingFocus() { IsEditButtonFocused(true); };
        void EditButtonLosingFocus() { IsEditButtonFocused(false); };
        bool ShowEditButton() const noexcept;
        void ToggleEditMode();
        void DisableEditMode() { IsInEditMode(false); }
        void AttemptAcceptChanges();
        void AttemptAcceptChanges(const Control::KeyChord newKeys);
        void CancelChanges();
        void DeleteKeyBinding() { DeleteKeyBindingRequested.raise(*this, _CurrentKeys); }

        // ProposedAction:   the entry selected by the combo box; may disagree with the settings model.
        // CurrentAction:    the combo box item that maps to the settings model value.
        // AvailableActions: the list of options in the combo box; both actions above must be in this list.
        // NOTE: ProposedAction and CurrentAction may disagree mainly due to the "edit mode" system in place.
        //       Current Action serves as...
        //       1 - a record of what to set ProposedAction to on a cancellation
        //       2 - a form of translation between ProposedAction and the settings model
        //       We would also need an ActionMap reference to remove this, but this is a better separation
        //       of responsibilities.
        VIEW_MODEL_OBSERVABLE_PROPERTY(IInspectable, ProposedAction);
        VIEW_MODEL_OBSERVABLE_PROPERTY(hstring, CurrentAction);
        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<hstring>, AvailableActions, nullptr);

        // ProposedKeys: the keys proposed by the control; may disagree with the settings model.
        // CurrentKeys:  the key chord bound in the settings model.
        VIEW_MODEL_OBSERVABLE_PROPERTY(Control::KeyChord, ProposedKeys);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Control::KeyChord, CurrentKeys, nullptr);

        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsInEditMode, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsNewlyAdded, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Windows::UI::Xaml::Controls::Flyout, AcceptChangesFlyout, nullptr);
        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsAutomationPeerAttached, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsHovered, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsContainerFocused, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsEditButtonFocused, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Windows::UI::Xaml::Media::Brush, ContainerBackground, nullptr);

    public:
        til::typed_event<Editor::KeyBindingViewModel, Editor::ModifyKeyBindingEventArgs> ModifyKeyBindingRequested;
        til::typed_event<Editor::KeyBindingViewModel, Terminal::Control::KeyChord> DeleteKeyBindingRequested;
        til::typed_event<Editor::KeyBindingViewModel, IInspectable> DeleteNewlyAddedKeyBinding;

    private:
        hstring _KeyChordText{};
    };

    struct CommandViewModel : CommandViewModelT<CommandViewModel>, ViewModelHelper<CommandViewModel>
    {
    public:
        CommandViewModel(winrt::Microsoft::Terminal::Settings::Model::Command cmd,
                         std::vector<Control::KeyChord> keyChordList,
                         const Windows::Foundation::Collections::IObservableVector<hstring>& availableActions,
                         const Editor::ActionsViewModel actionsPageVM,
                         const Windows::Foundation::Collections::IMap<Model::ShortcutAction, winrt::hstring>& availableShortcutActionsAndNames);
        void Initialize();

        winrt::hstring Name();
        void Name(const winrt::hstring& newName);

        winrt::hstring ID();
        void ID(const winrt::hstring& newID);

        bool IsUserAction();

        void Edit_Click();
        til::typed_event<Editor::CommandViewModel, IInspectable> EditRequested;

        void Delete_Click();
        til::typed_event<Editor::CommandViewModel, IInspectable> DeleteRequested;

        void AddKeybinding_Click();

        til::typed_event<IInspectable, Editor::ArgWrapper> PropagateColorSchemeRequested;

        VIEW_MODEL_OBSERVABLE_PROPERTY(hstring, CurrentAction);
        VIEW_MODEL_OBSERVABLE_PROPERTY(IInspectable, ProposedShortcutAction);
        VIEW_MODEL_OBSERVABLE_PROPERTY(hstring, CurrentShortcutAction);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Editor::ActionArgsViewModel, ActionArgsVM, nullptr);
        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<hstring>, AvailableActions, nullptr);
        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<hstring>, AvailableShortcutActions, nullptr);
        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<Editor::KeyChordViewModel>, KeyChordViewModelList, nullptr);

    private:
        winrt::Microsoft::Terminal::Settings::Model::Command _command;
        std::vector<Control::KeyChord> _keyChordList;
        weak_ref<Editor::ActionsViewModel> _actionsPageVM{ nullptr };
        void _RegisterKeyChordVMEvents(Editor::KeyChordViewModel kcVM);
        void _RegisterActionArgsVMEvents(Editor::ActionArgsViewModel actionArgsVM);
        Windows::Foundation::Collections::IMap<Model::ShortcutAction, winrt::hstring> _AvailableActionsAndNamesMap;
        std::unordered_map<winrt::hstring, Model::ShortcutAction> _NameToActionMap;
    };

    struct ArgWrapper : ArgWrapperT<ArgWrapper>, ViewModelHelper<ArgWrapper>
    {
    public:
        ArgWrapper(const winrt::hstring& name, const winrt::hstring& type, const bool required, const Windows::Foundation::IInspectable& value);
        void Initialize();

        winrt::hstring Name() const noexcept { return _name; };

        winrt::hstring Type() const noexcept { return _type; };

        bool Required() const noexcept { return _required; };

        Windows::Foundation::Collections::IObservableVector<Microsoft::Terminal::Settings::Editor::EnumEntry> EnumList() const noexcept { return _EnumList; };

        Windows::Foundation::Collections::IObservableVector<Microsoft::Terminal::Settings::Editor::FlagEntry> FlagList() const noexcept { return _FlagList; };

        void StringBindBack(const winrt::hstring& newValue) { Value(box_value(newValue)); };

        void DoubleBindBack(const double newValue) { Value(box_value(static_cast<uint32_t>(newValue))); };

        void DoubleOptionalBindBack(const double newValue) { Value(box_value(static_cast<uint32_t>(newValue))); };

        void FloatBindBack(const double newValue) { Value(box_value(static_cast<float>(newValue))); };

        void BoolBindBack(const Windows::Foundation::IReference<bool> newValue)
        {
            if (newValue)
            {
                Value(box_value(newValue));
            }
            else
            {
                Value(nullptr);
            }
        }

        winrt::Windows::Foundation::IReference<Microsoft::Terminal::Core::Color> UnboxTerminalCoreColorOptional(const Windows::Foundation::IInspectable& value)
        {
            if (value)
            {
                return unbox_value<Windows::Foundation::IReference<Microsoft::Terminal::Core::Color>>(value);
            }
            else
            {
                return nullptr;
            }
        }

        void TerminalCoreColorBindBack(const winrt::Windows::Foundation::IReference<Microsoft::Terminal::Core::Color> newValue)
        {
            if (newValue)
            {
                Value(box_value(newValue));
            }
            else
            {
                Value(nullptr);
            }
        }

        winrt::Windows::Foundation::IReference<Microsoft::Terminal::Core::Color> UnboxWindowsUIColorOptional(const Windows::Foundation::IInspectable& value)
        {
            if (value)
            {
                const auto winUIColor = unbox_value<Windows::Foundation::IReference<Windows::UI::Color>>(value).Value();
                const Microsoft::Terminal::Core::Color terminalColor{ winUIColor.R, winUIColor.G, winUIColor.B, winUIColor.A };
                return Windows::Foundation::IReference<Microsoft::Terminal::Core::Color>{ terminalColor };
            }
            else
            {
                return nullptr;
            }
        }

        void WindowsUIColorBindBack(const winrt::Windows::Foundation::IReference<Microsoft::Terminal::Core::Color> newValue)
        {
            if (newValue)
            {
                const auto terminalCoreColor = unbox_value<Windows::Foundation::IReference<Microsoft::Terminal::Core::Color>>(newValue).Value();
                const Windows::UI::Color winuiColor{
                    .A = terminalCoreColor.A,
                    .R = terminalCoreColor.R,
                    .G = terminalCoreColor.G,
                    .B = terminalCoreColor.B
                };
                // only set to the new value if our current value is not the same
                // unfortunately the Value setter does not do this check properly since
                // we create a whole new IReference even for the same underlying color
                if (_Value)
                {
                    const auto currentValue = unbox_value<Windows::Foundation::IReference<Windows::UI::Color>>(_Value).Value();
                    if (currentValue == winuiColor)
                    {
                        return;
                    }
                }
                Value(box_value(Windows::Foundation::IReference<Windows::UI::Color>{ winuiColor }));
            }
            else
            {
                Value(nullptr);
            }
        }

        // We cannot use the macro here because we need to implement additional logic for the setter
        Windows::Foundation::IInspectable EnumValue() const noexcept { return _EnumValue; };
        void EnumValue(const Windows::Foundation::IInspectable& value);

        til::typed_event<IInspectable, Editor::ArgWrapper> ColorSchemeRequested;

        VIEW_MODEL_OBSERVABLE_PROPERTY(Editor::ColorSchemeViewModel, DefaultColorScheme, nullptr);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Windows::Foundation::IInspectable, Value, nullptr);

    private:
        winrt::hstring _name;
        winrt::hstring _type;
        bool _required;
        Windows::Foundation::IInspectable _EnumValue{ nullptr };
        Windows::Foundation::Collections::IObservableVector<Microsoft::Terminal::Settings::Editor::EnumEntry> _EnumList;
        Windows::Foundation::Collections::IObservableVector<Microsoft::Terminal::Settings::Editor::FlagEntry> _FlagList;
    };

    struct ActionArgsViewModel : ActionArgsViewModelT<ActionArgsViewModel>, ViewModelHelper<ActionArgsViewModel>
    {
    public:
        ActionArgsViewModel(const Microsoft::Terminal::Settings::Model::ActionAndArgs actionAndArgs);
        void Initialize();

        til::typed_event<IInspectable, Editor::ArgWrapper> PropagateColorSchemeRequested;

        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<Editor::ArgWrapper>, ArgValues, nullptr);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Microsoft::Terminal::Settings::Model::ShortcutAction, ShortcutActionType, Microsoft::Terminal::Settings::Model::ShortcutAction::AddMark);

    private:
        Model::ActionAndArgs _actionAndArgs{ nullptr };
        winrt::hstring _StringArg1;
        uint32_t _UInt32Arg1;
        bool _BoolArg1;
    };

    struct KeyChordViewModel : KeyChordViewModelT<KeyChordViewModel>, ViewModelHelper<KeyChordViewModel>
    {
    public:
        KeyChordViewModel(Control::KeyChord CurrentKeys);

        void CurrentKeys(const Control::KeyChord& newKeys);
        Control::KeyChord CurrentKeys() const noexcept;

        void ToggleEditMode();
        void AttemptAcceptChanges();
        void CancelChanges();
        void DeleteKeyChord();

        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsInEditMode, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Control::KeyChord, ProposedKeys);
        VIEW_MODEL_OBSERVABLE_PROPERTY(winrt::hstring, KeyChordText);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Windows::UI::Xaml::Controls::Flyout, AcceptChangesFlyout, nullptr);

    public:
        til::typed_event<Editor::KeyChordViewModel, Terminal::Control::KeyChord> AddKeyChordRequested;
        til::typed_event<Editor::KeyChordViewModel, Editor::ModifyKeyChordEventArgs> ModifyKeyChordRequested;
        til::typed_event<Editor::KeyChordViewModel, Terminal::Control::KeyChord> DeleteKeyChordRequested;

    private:
        Control::KeyChord _currentKeys;
    };

    struct ActionsViewModel : ActionsViewModelT<ActionsViewModel>, ViewModelHelper<ActionsViewModel>
    {
    public:
        ActionsViewModel(Model::CascadiaSettings settings);
        void UpdateSettings(const Model::CascadiaSettings& settings);

        void OnAutomationPeerAttached();
        void AddNewKeybinding();

        void CurrentCommand(const Editor::CommandViewModel& newCommand);
        Editor::CommandViewModel CurrentCommand();

        void AttemptModifyKeyChord(const Editor::KeyChordViewModel& senderVM, const Editor::ModifyKeyChordEventArgs& args);
        void AttemptDeleteKeyChord(const Control::KeyChord& keys);
        void AttemptAddKeyChord(const Control::KeyChord& keys, const winrt::hstring& cmdID);

        til::typed_event<IInspectable, IInspectable> FocusContainer;
        til::typed_event<IInspectable, IInspectable> UpdateBackground;

        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<Editor::KeyBindingViewModel>, KeyBindingList);
        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<Editor::CommandViewModel>, CommandList);
        WINRT_OBSERVABLE_PROPERTY(ActionsSubPage, CurrentPage, _propertyChangedHandlers, ActionsSubPage::Base);

    private:
        Editor::CommandViewModel _CurrentCommand{ nullptr };
        bool _AutomationPeerAttached{ false };
        Model::CascadiaSettings _Settings;
        Windows::Foundation::Collections::IObservableVector<hstring> _AvailableActionAndArgs;
        Windows::Foundation::Collections::IMap<hstring, Model::ActionAndArgs> _AvailableActionMap;
        Windows::Foundation::Collections::IMap<Model::ShortcutAction, winrt::hstring> _AvailableActionsAndNamesMap;

        void _MakeCommandVMsHelper();

        std::optional<uint32_t> _GetContainerIndexByKeyChord(const Control::KeyChord& keys);
        void _RegisterEvents(com_ptr<implementation::KeyBindingViewModel>& kbdVM);
        void _RegisterCmdVMEvents(com_ptr<implementation::CommandViewModel>& cmdVM);

        void _KeyBindingViewModelPropertyChangedHandler(const Windows::Foundation::IInspectable& senderVM, const Windows::UI::Xaml::Data::PropertyChangedEventArgs& args);
        void _KeyBindingViewModelDeleteKeyBindingHandler(const Editor::KeyBindingViewModel& senderVM, const Control::KeyChord& args);
        void _KeyBindingViewModelModifyKeyBindingHandler(const Editor::KeyBindingViewModel& senderVM, const Editor::ModifyKeyBindingEventArgs& args);
        void _KeyBindingViewModelDeleteNewlyAddedKeyBindingHandler(const Editor::KeyBindingViewModel& senderVM, const IInspectable& args);

        void _CmdVMPropertyChangedHandler(const IInspectable& sender, const Windows::UI::Xaml::Data::PropertyChangedEventArgs& args);
        void _CmdVMEditRequestedHandler(const Editor::CommandViewModel& senderVM, const IInspectable& args);
        void _CmdVMDeleteRequestedHandler(const Editor::CommandViewModel& senderVM, const IInspectable& args);
        void _CmdVMPropagateColorSchemeRequestedHandler(const IInspectable& sender, const Editor::ArgWrapper& wrapper);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(ActionsViewModel);
}
