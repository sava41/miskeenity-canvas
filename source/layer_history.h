#pragma once

#include "layer_manager.h"

#include <memory>

namespace mc
{
    // memento implmention of layer history using a ring buffer
    class LayerHistory
    {
      public:
        LayerHistory( size_t maxLayers );
        ~LayerHistory() = default;

        void push( LayerManager&& memento );

        const LayerManager& undo();
        const LayerManager& redo();

        bool atFront() const;
        bool atBack() const;

        void setCheckpoint();
        void removeCheckpoint();
        const LayerManager& resetToCheckpoint();

        size_t length() const;

      private:
        bool m_full;
        size_t m_maxLength;
        size_t m_front;
        size_t m_back;
        size_t m_currentMemento;

        int m_checkpoint;

        std::unique_ptr<LayerManager[]> m_mementos;
    };

} // namespace mc