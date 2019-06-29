#ifndef PAGE_H
#define PAGE_H

#include "FlashTypes.h"

namespace NVM {
  namespace FlashMemory {
    // ------------------
    // PageMetadata Class
    // ------------------
    class PageMetadata {
    public:
      // TODO Hide member variable
      LPA_type LPA;
      // page_status_type Status;
      // stream_id_type SourceStreamID;

    public:
      PageMetadata();

      void update(const PageMetadata& metadata);
    };

    force_inline
    PageMetadata::PageMetadata()
      : LPA(NO_LPA)
    {
      // Currently not in use.
      //
      // Status = FREE_PAGE;
      // SourceStreamID = NO_STREAM;
    }

    force_inline void
    PageMetadata::update(const PageMetadata& metadata)
    {
      LPA = metadata.LPA;

      // Status = metadata.Status;
      // SourceStreamID = metadata.SourceStreamID;
    }

    // ----------
    // Page Class
    // ----------
    class Page {
    public:
      // TODO Hide member variable
      PageMetadata Metadata;

    public:
      Page();

      void Write_metadata(const PageMetadata& metadata);
      void Read_metadata(PageMetadata& metadata) const;
    };

    force_inline
    Page::Page()
      : Metadata()
    { }

    force_inline void
    Page::Write_metadata(const PageMetadata& metadata)
    {
      Metadata.update(metadata);
    }

    force_inline void
    Page::Read_metadata(PageMetadata& metadata) const
    {
      metadata.update(Metadata);
    }

  }
}

#endif // !PAGE_H
