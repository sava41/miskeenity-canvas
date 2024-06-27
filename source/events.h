#pragma once

namespace mc
{

    enum class Events
    {
        ChangeMode,
        SelectionRequested,
        SelectionChanged,
        SelectionReady,
        Scale,
        Rotate,
        FlipHorizontal,
        FlipVertical,
        MoveFront,
        MoveBack,
        Delete,
        Undo,
        Redo,
        LoadImage,
        SaveImage
    };

    enum class EventDataType
    {
        None,
        ImageData
    };

    void submitEvent( const Events& event, const EventDataType& type = EventDataType::None, void* userData = nullptr );

} // namespace mc