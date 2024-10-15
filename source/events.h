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
        SelectionRequested,
        SelectionChanged,
        SelectionReady,
        FlipHorizontal,
        FlipVertical,
        MoveFront,
        MoveBack,
        Crop,
        Cut,
        Segment,
        Delete,
        Undo,
        Redo,
        LoadImage,
        SaveImageRequest,
        SaveImage,
        AppQuit,
        OpenGithub,
        ChangeMode,
        StartCrop,
        MergeEditLayers,
        ResetEditLayers,
        AddMergedLayer,
        MergeAndRasterize,
        DeleteEditLayers
    };

    void submitEvent( const Events& event, const EventData& data = {} );

} // namespace mc