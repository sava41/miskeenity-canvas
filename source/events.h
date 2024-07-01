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
        Delete,
        Undo,
        Redo,
        LoadImage,
        SaveImage,
        AppQuit,
        OpenGithub
    };

    enum class EventDataType
    {
        None,
        ImageData
    };

    void submitEvent( const Events& event, const EventDataType& type = EventDataType::None, void* userData = nullptr );

} // namespace mc