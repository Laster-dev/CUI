#pragma once

#include "catalog/Catalog.h"
#include <vector>

namespace Gallery {

class BasicInputCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class CollectionsCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class DateAndTimeCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class DialogsAndFlyoutsCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class LayoutCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class MediaCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class MenusAndToolbarsCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class MotionCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class NavigationCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class ScrollingCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class StatusAndInfoCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class StylesCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class SystemCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class TextCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

class WindowingCatalog {
public:
    static void Register(std::vector<Entry>& entries);
};

} // namespace Gallery
