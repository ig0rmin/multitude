/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Export.hpp"
#include "CPUMipmapStore.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous {

  using namespace Radiant;

  static Radiant::Mutex s_mutex;

  static std::map<std::string, std::weak_ptr<CPUMipmaps> > s_mipmaps;

  std::shared_ptr<CPUMipmaps> CPUMipmapStore::acquire(const std::string & filename, bool immediate)
  {
    Radiant::Guard g( s_mutex);

    std::weak_ptr<CPUMipmaps> & mipmap_weak = s_mipmaps[filename];

    // Check if ptr still points to something valid
    std::shared_ptr<CPUMipmaps> mipmap_shared = mipmap_weak.lock();
    if (mipmap_shared)
      return mipmap_shared;

    mipmap_shared.reset(new CPUMipmaps);

    if(!mipmap_shared->startLoading(filename.c_str(), immediate)) {
      return std::shared_ptr<CPUMipmaps>();
    }

    /// @todo fix
    static std::vector<std::shared_ptr<CPUMipmaps> > sharedptrs;
    std::shared_ptr<CPUMipmaps> s(mipmap_shared);
    sharedptrs.push_back(s);

    Luminous::BGThread::instance()->addTask(mipmap_shared.get());

    // store new weak pointer
    mipmap_weak = mipmap_shared;

    debugLuminous("CPUMipmapStore::acquire # Created new for %s (%ld links)",
          filename.c_str(), s_mipmaps[filename].use_count());

    return mipmap_shared;
  }

  void CPUMipmapStore::release(std::shared_ptr<CPUMipmaps>)
  {
    /*if(!mipmaps)
      return;

    Radiant::Guard g( s_mutex);

    for(MipMapItemContainer::iterator it = s_mipmaps.begin();
    it != s_mipmaps.end(); it++) {
      MipmapItem & mmi = (*it).second;
      if(mmi.m_mipmaps == mipmaps) {
        mmi.decrCount();
        if(!mmi.m_linkCount) {
          // info("Erased mipmaps %p", mipmaps);
          s_mipmaps.erase(it);
        }
        return;
      }
    }*/
  }

  std::shared_ptr<CPUMipmaps> CPUMipmapStore::copy(std::shared_ptr<CPUMipmaps> mipmaps)
  {
    return std::shared_ptr<CPUMipmaps>(mipmaps);
    /*if(!mipmaps)
      return 0;

    Radiant::Guard g( s_mutex);

    for(MipMapItemContainer::iterator it = s_mipmaps.begin();
    it != s_mipmaps.end(); it++) {
      MipmapItem & mmi = (*it).second;
      if(mmi.m_mipmaps == mipmaps) {
        mmi.incrCount();
        return mipmaps;
      }
    }
    return 0;*/
  }

  unsigned CPUMipmapStore::count()
  {
    Radiant::Guard g( s_mutex);
    return (unsigned) s_mipmaps.size();
  }

}
