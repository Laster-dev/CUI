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
    static void ScanNetworkProviders(ScanSummary& result);
    static void ScanRunOnceEx(HKEY root, bool isHklm, const char* subKey, const char* scope, ScanSummary& result);
    static void ScanShellServiceObjects(ScanSummary& result);
    static void ScanBrowserHelpers(ScanSummary& result);
    static void ScanOfficeAddins(ScanSummary& result);
    static void ScanSafeMode(ScanSummary& result);
    static void ScanProtocolHandlers(ScanSummary& result);
    static void ScanDirectShowFilters(ScanSummary& result);
    static void ScanSharedTaskSchedulers(ScanSummary& result);
    static void ScanUrlSearchHooks(ScanSummary& result);
    static void ScanDrivers32Codecs(ScanSummary& result);
    static void ScanPrintProcessors(ScanSummary& result);
    static void ScanSecurityProviders(ScanSummary& result);
    static void ScanScreenSaver(ScanSummary& result);
    static void ScanAppCertDlls(ScanSummary& result);
    static void ScanTerminalServer(ScanSummary& result);
    static void ScanUserInitMprLogonScript(ScanSummary& result);
    static void ScanPrintProviders(ScanSummary& result);
    static void ScanShellExecuteHooks(ScanSummary& result);
    static void ScanChromiumExtensions(ScanSummary& result);
    static void ScanOfficeStartupFolders(ScanSummary& result);

    static void AddEntry(ScanSummary& result, StartupEntry entry);
    static StartupEntry Analyze(StartupEntry entry);
    static void ExtractVersionInfo(const std::wstring& filePath, std::string& outCompany, std::string& outDesc, std::string& outVersion);
    static std::string FormatFileSize(uint64_t bytes);
    static std::string FormatFileTime(const FILETIME& ft);
};

} // namespace AutoGuard
