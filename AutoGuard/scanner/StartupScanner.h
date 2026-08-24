#pragma once

#include "../model/StartupEntry.h"
#include <windows.h>
#include <taskschd.h>
#include <string>
#include <vector>

namespace AutoGuard {

class StartupScanner {
public:
    ScanSummary Scan() const;

public:
    static void ScanRegistryRoot(HKEY root, const char* scope, const char* subKey, bool isHklm, StartupCategory cat, StartupLocation loc, const std::string& groupTitle, ScanSummary& result);
    static void ScanStartupFolder(const std::wstring& folder, const char* scope, ScanSummary& result);
    static void ScanScheduledTasks(ScanSummary& result);
    static void ScanServices(ScanSummary& result);
    static void ScanDrivers(ScanSummary& result);
    static void ScanImageHijacks(ScanSummary& result);
    static void ScanWinlogon(ScanSummary& result);
    static void ScanActiveSetup(ScanSummary& result);
    static void ScanComExtensions(ScanSummary& result);
    static void ScanAppInitDlls(ScanSummary& result);
    static void ScanKnownDlls(ScanSummary& result);
    static void ScanBootExecute(ScanSummary& result);
    static void ScanWinsockProviders(ScanSummary& result);
    static void ScanPrintMonitors(ScanSummary& result);
    static void ScanLsaPackages(ScanSummary& result);
    static void ScanWmiSubscriptions(ScanSummary& result);

    static void AddEntry(ScanSummary& result, StartupEntry entry);
    static StartupEntry Analyze(StartupEntry entry);
    static void ExtractVersionInfo(const std::wstring& filePath, std::string& outCompany, std::string& outDesc, std::string& outVersion);
    static std::string FormatFileSize(uint64_t bytes);
    static std::string FormatFileTime(const FILETIME& ft);
};

} // namespace AutoGuard
