// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "ActionsViewModel.g.h"
#include "KeyBindingViewModel.g.h"
#include "CommandViewModel.g.h"
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
                         const Editor::ActionsViewModel actionsPageVM);

        winrt::hstring Name();
        void Name(const winrt::hstring& newName);

        winrt::hstring ID();
        void ID(const winrt::hstring& newID);

        bool IsUserAction();

        void Edit_Click();
        til::typed_event<Editor::CommandViewModel, IInspectable> EditRequested;

        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<Editor::KeyChordViewModel>, KeyChordViewModelList, nullptr);

    private:
        winrt::Microsoft::Terminal::Settings::Model::Command _command;
        weak_ref<Editor::ActionsViewModel> _actionsPageVM{ nullptr };
        void _RegisterEvents(Editor::KeyChordViewModel kcVM);
    };

    struct KeyChordViewModel : KeyChordViewModelT<KeyChordViewModel>, ViewModelHelper<KeyChordViewModel>
    {
    public:
        KeyChordViewModel(Control::KeyChord CurrentKeys);

        hstring KeyChordText() { return Model::KeyChordSerialization::ToString(_CurrentKeys); };

        void ToggleEditMode();
        void AttemptAcceptChanges();
        void CancelChanges();

        VIEW_MODEL_OBSERVABLE_PROPERTY(bool, IsInEditMode, false);
        VIEW_MODEL_OBSERVABLE_PROPERTY(Control::KeyChord, ProposedKeys);

    public:
        til::typed_event<Editor::KeyChordViewModel, Editor::ModifyKeyChordEventArgs> ModifyKeyChordRequested;

    private:
        Control::KeyChord _CurrentKeys;
    };

    struct ActionsViewModel : ActionsViewModelT<ActionsViewModel>, ViewModelHelper<ActionsViewModel>
    {
    public:
        ActionsViewModel(Model::CascadiaSettings settings);

        void OnAutomationPeerAttached();
        void AddNewKeybinding();

        void CurrentCommand(const Editor::CommandViewModel& newCommand);
        Editor::CommandViewModel CurrentCommand();

        void AttemptModifyKeyBinding(const Editor::KeyChordViewModel& senderVM, const Editor::ModifyKeyChordEventArgs& args);

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

        std::optional<uint32_t> _GetContainerIndexByKeyChord(const Control::KeyChord& keys);
        void _RegisterEvents(com_ptr<implementation::KeyBindingViewModel>& kbdVM);
        void _RegisterCmdVMEvents(com_ptr<implementation::CommandViewModel>& cmdVM);

        void _KeyBindingViewModelPropertyChangedHandler(const Windows::Foundation::IInspectable& senderVM, const Windows::UI::Xaml::Data::PropertyChangedEventArgs& args);
        void _KeyBindingViewModelDeleteKeyBindingHandler(const Editor::KeyBindingViewModel& senderVM, const Control::KeyChord& args);
        void _KeyBindingViewModelModifyKeyBindingHandler(const Editor::KeyBindingViewModel& senderVM, const Editor::ModifyKeyBindingEventArgs& args);
        void _KeyBindingViewModelDeleteNewlyAddedKeyBindingHandler(const Editor::KeyBindingViewModel& senderVM, const IInspectable& args);

        void _CmdVMEditRequestedHandler(const Editor::CommandViewModel& senderVM, const IInspectable& args);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(ActionsViewModel);
}
