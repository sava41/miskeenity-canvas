#pragma once

#include "app.h"

#include <SDL3/SDL_events.h>

namespace mc
{
    struct EventData
    {
        Mode mode;
    };

    // use extra space in SDL_Event union to store our own data
    struct Event
    {
        SDL_UserEvent sdlUserEvent;

        EventData data;
    };
    static_assert( sizeof( Event ) <= sizeof( SDL_Event ) );

    enum class Events
    {
        SelectionChanged,
        ComputeSelectionBbox,
        FlipHorizontal,
        FlipVertical,
        MoveFront,
        MoveBack,
        ApplyCut,
        Delete,
        Undo,
        Redo,
        LoadImage,
        SaveImageRequest,
        SaveImage,
        AppQuit,
        OpenGithub,
        ChangeMode,
        ApplyCrop,
        MergeEditLayers,
        ResetEditLayers,
        AddMergedLayer,
        MergeAndRasterizeRequest,
        MergeAndRasterize,
        SamLoadInput,
        SamUploadMask,
        AddImageToLayer,
    };

    void submitEvent( const Events& event, const EventData& data = {}, void* ptrData = nullptr );

} // namespace mc