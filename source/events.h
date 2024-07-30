#pragma once

namespace mc
{

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
        Delete,
        Undo,
        Redo,
        LoadImage,
        SaveImage,
        AppQuit,
        OpenGithub
    };

    void submitEvent( const Events& event, void* userData = nullptr );

} // namespace mc