#pragma once

#include <memory>
#include <unordered_set>

namespace mc
{
    class ResourceManager;

    // used to reference count and retrieve resources from resource manager derived classes
    class ResourceHandle
    {
        friend class ResourceManager;

      public:
        ~ResourceHandle();
        static ResourceHandle invalidResource();

        bool valid() const;
        int resourceIndex() const;

        ResourceHandle( const ResourceHandle& handle );
        ResourceHandle( ResourceHandle&& handle );

        ResourceHandle& operator=( ResourceHandle& )  = delete;
        ResourceHandle& operator=( ResourceHandle&& ) = delete;

      private:
        ResourceHandle( ResourceManager* manager, int index );

        ResourceManager* m_manager;
        bool m_valid;
        int m_resourceIndex;
    };

    // base class used in conjunction with ResourceHandle to reference count and store shared resources
    class ResourceManager
    {
        friend class ResourceHandle;

      public:
        ResourceManager( size_t maxLength );
        ~ResourceManager();

        size_t maxLength() const;
        size_t curLength() const;

        ResourceManager( const ResourceManager& handle ) = delete;
        ResourceManager( ResourceManager&& handle )      = delete;

        ResourceManager& operator=( ResourceManager& )  = delete;
        ResourceManager& operator=( ResourceManager&& ) = delete;

      protected:
        ResourceHandle getHandle( int resourceIndex );
        int getRefCount( int resourceIndex );
        virtual void freeResource( int resourceIndex ) = 0;

      private:
        size_t m_length;
        size_t m_maxLength;

        std::unique_ptr<int[]> m_references;
        std::unordered_set<ResourceHandle*> m_handles;
    };

} // namespace mc