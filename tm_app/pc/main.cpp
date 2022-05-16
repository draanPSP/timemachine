#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>
#include <string>
#include <string.h>
#include <getopt.h>

#include "pspdecrypt_lib.h"
#include "PsarDecrypter.h"
#include "PrxDecrypter.h"
#include <common.h>

#include "tm_sloader.h"
#include "100_payload.h"
#include "100_tmctrl100.h"
#include "100B_payload.h"
#include "100B_tmctrl100B.h"
#include "100B_vshex.h"
#include "150_payload.h"
#include "150_tmctrl150.h"
#include "200_payload.h"
#include "200_tmctrl200.h"
#include "250_payload.h"
#include "250_tmctrl250.h"
#include "260_payload.h"
#include "260_tmctrl260.h"
#include "271_payload.h"
#include "271_tmctrl150.h"
#include "271_tmctrl271.h"
#include "271_systemctrl150.h"
#include "271_systemctrl.h"
#include "271_paf.h"
#include "271_vshmain.h"
#include "280_payload.h"
#include "280_tmctrl280.h"
#include "340_payload.h"
#include "340_tmctrl150.h"
#include "340_tmctrl340.h"
#include "340_idcanager.h"
#include "340_lcpatcher.h"
#include "340_popcorn.h"
#include "340_reboot150.h"
#include "340_recovery.h"
#include "340_systemctrl.h"
#include "340_systemctrl150.h"
#include "340_vshctrl.h"

using namespace std;
namespace fs = std::filesystem;

void help(const char* exeName) {
    cout << "Usage: " << exeName << " [OPTION]..." << endl;
    cout << endl;
    cout << "Creates TM firmware directory for the specified version." << endl;
    cout << endl;
    cout << "General options:" << endl;
    cout << "  -h, --help              display this help and exit" << endl;
    cout << "  -v, --verbose           enable verbose mode (mostly for debugging)" << endl;
    cout << "  -t, --tmdir=DIR         path to TM directory" << endl;
    cout << "  -u, --updir=DIR         path to updater pbps" << endl;
    cout << "  -V, --version=VER       TM firmware version. Supported versions are: 1.00, 1.00Bogus, 1.50, 2.00, 2.50, 2.60, 2.71SE-C, 2.80, 3.40OE-A" << endl;
    cout << "  -d, --downdaterdir=DIR  path to 1.00 downdater dump. Required for 1.00 and 1.00Bogus firmware installs" << endl;
    cout << "  -b, --bogusfix          install module to fix corrupt eboots in 1.00 Bogus firmware" << endl;
}

int readFile(string path, char **inData) {
    ifstream inFile (path, ios::in|ios::binary|ios::ate);
    if (!inFile.is_open()) {
        cerr << "Could not open " << path << "!" << endl;
        return -1;
    }

    streampos size = inFile.tellg();
    *inData = new char[size];
    inFile.seekg(0, ios::beg);
    inFile.read(*inData, size);
    inFile.close();
    if (size < 0x30) {
        cerr << "Input file is too small!" << endl;
        return -1;
    }

    return size;
}

void extract100Ipl(string iplPath, string installPath, bool verbose) {
    
    string logStr;
    string tmpDir = installPath + "/tmp";
    char *buf;

    fs::create_directory(tmpDir); 

    int size = readFile(iplPath, &buf);
    decryptIPL((u8*)buf, size, 100, "psp_ipl.bin", tmpDir, nullptr, 0, verbose, false, logStr);
    cout << "Decrypting 1.00 ipl" << logStr << endl;  

    fs::copy(tmpDir + "/stage1_psp_ipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);

    fs::remove_all(tmpDir);

    delete[] buf;
}

void extract150Ipl(u8 *dataPsp, u32 size, string installPath, bool verbose) {
    
    string logStr;
    char *outData = new char[0x400000];
    
    // decrypt updater
    int outSize = pspDecryptPRX(dataPsp, (u8 *)outData, size, nullptr, true);
    char *updaterData = new char[outSize];

    // extract ipl from ipl updater module
    cout << "Extracting ipl" << endl;
    outSize = pspDecryptPRX((const u8 *)&outData[0x303100], (u8 *)updaterData, 0x38480, nullptr, true);
    decryptIPL((u8*)&updaterData[0x980], 0x37000, 150, "psp_ipl.bin", installPath + "/PSARDUMPER", nullptr, 0, verbose, false, logStr);
    cout << logStr << endl;

    fs::copy(installPath + "/PSARDUMPER/stage1_psp_ipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);

    delete[] outData;
    delete[] updaterData;
}

int decryptModule(string modulePath, char** outData) {

    char *inData;
    *outData = new char[0x400000];

    int size = readFile(modulePath, &inData);
    size = pspDecryptPRX((u8 *)inData, (u8 *)*outData, size, nullptr, true);

    delete[] inData;

    return size;
}

void updateBtCfg(string path, vector<pair<string, string>> replacements) {
    
    char *cfgBuf;
    int cfgSize = decryptModule(path, &cfgBuf);
    cfgBuf[cfgSize] = 0;
    string btCnf(cfgBuf);

    for (int i = 0; i < replacements.size(); i++) {
        btCnf = regex_replace(btCnf, regex(replacements[i].first), replacements[i].second);
    }
    
    WriteFile((path).c_str(), (void*)btCnf.c_str(), btCnf.length());

    delete[] cfgBuf;
}

void writeFileWithEmbeddedReboot(string filePath, string rebootBinPath, const unsigned char *fileBuf, int fileSize, int rebootBinOffset, bool gzip) 
{
    char *rebootBuf;
    char *outBuf = new char[fileSize];
    int rebootSize = readFile(rebootBinPath, &rebootBuf);
    char *compressedBuf = new char[rebootSize];
    int compressedSize;

    compressedSize = deflateCompress((void*)rebootBuf, rebootSize, (void*)compressedBuf, rebootSize, 7, gzip);

    memcpy(outBuf, fileBuf, fileSize);
    memcpy(&outBuf[rebootBinOffset], compressedBuf, compressedSize);

    WriteFile((filePath).c_str(), (void*)outBuf, fileSize);

    delete[] rebootBuf;
    delete[] outBuf;
    delete[] compressedBuf;
}

void extractFirmware(string installPath, string upDir, string version, bool deletePsarDumper, bool copyIpl, bool verbose) {

    char *buf;
    string installerPath = upDir + "/" + version + ".PBP";

    int size = readFile(installerPath, &buf);
    if (size > 0) {
        cout << "Setting up firmware " << version <<" in " << installPath << endl;
        u32 pspOff = *(u32*)&buf[0x20];
        u32 psarOff = *(u32*)&buf[0x24];

        cout << "Extracting firmware files from update PBP" << endl;
        pspDecryptPSAR((u8*)&buf[psarOff], size - psarOff, installPath, true, nullptr, 0, verbose, false, false);

        if (copyIpl && version.compare("150") == 0) {
            extract150Ipl((u8*)&buf[pspOff], psarOff - pspOff, installPath, verbose);
        }
        else if (copyIpl && fs::exists(installPath + "/PSARDUMPER/stage1_psp_ipl.bin"))
            fs::copy(installPath + "/PSARDUMPER/stage1_psp_ipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);
        else if (copyIpl && fs::exists(installPath + "/PSARDUMPER/stage1_psp_nandipl.bin"))
            fs::copy(installPath + "/PSARDUMPER/stage1_psp_nandipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);

        if (deletePsarDumper)
            fs::remove_all(installPath + "/PSARDUMPER");

        // tidy up
        delete[] buf;
    }
    else
    {
        cerr << "Error reading " << installerPath << endl;
        exit(1);
    }
        
}

int main(int argc, char *argv[]) {

    static struct option long_options[] = {
        {"help",         no_argument,       0, 'h'},
        {"verbose",      no_argument,       0, 'v'},
        {"tmdir",        required_argument, 0, 't'},
        {"updir",        required_argument, 0, 'u'},
        {"version",      required_argument, 0, 'V'},
        {"downdaterdir", required_argument, 0, 'd'},
        {"bogusfix",     no_argument,       0, 'b'},
        {0,              0,                 0,  0  }
    };
    int long_index;

    string tmDir = "";
    string upDir = ".";
    bool verbose = false;
    string version;
    string downdaterDir;
    bool installBogusFix = false;
    int c = 0;
    cout << showbase << internal << setfill('0');
    while ((c = getopt_long(argc, argv, "hvo:t:u:V:d:b", long_options, &long_index)) != -1) {
        switch (c) {
            case 'h':
                help(argv[0]);
                return 0;
            case 'v':
                verbose = true;
                break;
            case 't':
                tmDir = string(optarg);
                break;
            case 'u':
                upDir = string(optarg);
                break;
            case 'V':
                version = string(optarg);
                break;
            case 'd':
                downdaterDir = string(optarg);
                break;
            case 'b':
                installBogusFix = true;
                break;
            default:
                help(argv[0]);
                return 1;
        }
    }

    char* inData;
    char* outData;
    int size;

    if (!fs::exists(tmDir)) {
        cerr << "Specified TM directory does not exist!" << endl;
        return 1;
    }

    if ((version.compare("1.00") == 0) || (version.compare("1.00Bogus") == 0))
    {
        if (downdaterDir.empty()) {
            cerr << "Downdater required for 1.00 firmware installs" << endl;
            return 1;
        }
        if (!fs::exists(downdaterDir)) {
            cerr << "Downdater directory not found" << endl;
            return 1;
        }
        if (!fs::exists(downdaterDir + "/ipl.bin") || 
            !fs::exists(downdaterDir + "DUMP")) {
            cerr << "Downdater directory does not contain expected contents" << endl;
            return 1;
        }
    }
    
    if (version.compare("1.00") == 0) {
        string installDir = tmDir + "/100";
        
        const auto copyOptions = fs::copy_options::overwrite_existing 
                               | fs::copy_options::recursive;

        fs::copy(downdaterDir + "/DUMP/", installDir, copyOptions);
        fs::create_directory(installDir + "registry");

        extract100Ipl(downdaterDir + "/ipl.bin", installDir, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm100_payload, sizeof(tm100_payload));
        WriteFile((installDir + "/tmctrl100.prx").c_str(), (void*)tm100_tmctrl100, sizeof(tm100_tmctrl100));
    }
    else if (version.compare("1.00Bogus") == 0) {
        string installDir = tmDir + "/100_Bogus";
        extractFirmware(installDir, upDir, "100", true, false, verbose);

        extract100Ipl(downdaterDir + "/ipl.bin", installDir, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm100B_payload, sizeof(tm100B_payload));
        WriteFile((installDir + "/tmctrl100_Bogus.prx").c_str(), (void*)tm100B_tmctrl100B, sizeof(tm100B_tmctrl100B));

        if (installBogusFix) {
            cout << "Installing corrupt eboot fix" << endl;
            updateBtCfg(installDir + "/kd/pspbtcnf.txt", { { "%/vsh/module/common_util.prx", "%/vsh/module/common_util.prx\n%/kd/vshex.prx" } });
            WriteFile((installDir + "/kd/vshex.prx").c_str(), (void*)tm100B_vshex, sizeof(tm100B_vshex));
        }
    }
    else if (version.compare("1.50") == 0) {
        string installDir = tmDir + "/150";
        extractFirmware(installDir, upDir, "150", true, true, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm150_payload, sizeof(tm150_payload));
        WriteFile((installDir + "/tmctrl150.prx").c_str(), (void*)tm150_tmctrl150, sizeof(tm150_tmctrl150));
    }
    else if (version.compare("2.00") == 0) {
        string installDir = tmDir + "/200";
        extractFirmware(installDir, upDir, "200", true, true, verbose);
        
        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm200_payload, sizeof(tm200_payload));
        WriteFile((installDir + "/tmctrl200.prx").c_str(), (void*)tm200_tmctrl200, sizeof(tm200_tmctrl200));
    }
    else if (version.compare("2.50") == 0) {
        string installDir = tmDir + "/250";
        extractFirmware(installDir, upDir, "250", true, true, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm250_payload, sizeof(tm250_payload));
        WriteFile((installDir + "/tmctrl250.prx").c_str(), (void*)tm250_tmctrl250, sizeof(tm250_tmctrl250));
    }
    else if (version.compare("2.60") == 0) {
        string installDir = tmDir + "/260";
        extractFirmware(installDir, upDir, "260", true, true, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm260_payload, sizeof(tm260_payload));
        WriteFile((installDir + "/tmctrl260.prx").c_str(), (void*)tm260_tmctrl260, sizeof(tm260_tmctrl260));
    }
    else if (version.compare("2.71SE-C") == 0) {
        string installDir = tmDir + "/271SE";
        string tmpDir = installDir + "/tmp";

        extractFirmware(installDir, upDir, "150", false, true, verbose);

        //Decrypt boot config files and insert systemctrl.prx
        updateBtCfg(installDir + "/kd/pspbtcnf_game.txt", { { "/kd/impose.prx", "/kd/impose.prx\n/kd/systemctrl.prx" } });
        updateBtCfg(installDir + "/kd/pspbtcnf_updater.txt", { { "/kd/vshbridge.prx", "/kd/vshbridge.prx\n/kd/systemctrl.prx" } });

        //Remove unused 1.50 firmware files
        fs::remove_all(installDir + "/vsh/etc/");
        fs::remove_all(installDir + "/vsh/resource/");
        fs::remove(installDir + "/kd/mebooter_umdvideo.prx");
        fs::remove(installDir + "/kd/reboot.prx");

        const char *plugins[11] = { "auth_plugin", "game_plugin", "impose_plugin", "msgdialog_plugin",
                                    "msvideo_plugin", "music_plugin", "opening_plugin", "photo_plugin",
                                    "sysconf_plugin", "update_plugin", "video_plugin" };

        for (int i = 0; i < 11; i++) {
            fs::remove(installDir + "/vsh/module/" + plugins[i] + ".prx");
        }

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm271_payload, sizeof(tm271_payload));
        WriteFile((installDir + "/tmctrl150.prx").c_str(), (void*)tm271_tmctrl150, sizeof(tm271_tmctrl150));
        WriteFile((installDir + "/tmctrl271.prx").c_str(), (void*)tm271_tmctrl271, sizeof(tm271_tmctrl271));
        WriteFile((installDir + "/vsh/module/paf.prx").c_str(), (void*)tm271_paf, sizeof(tm271_paf));
        WriteFile((installDir + "/vsh/module/vshmain.prx").c_str(), (void*)tm271_vshmain, sizeof(tm271_vshmain));

        //Setup 271
        extractFirmware(tmpDir, upDir, "271", false, false, verbose);

        const auto copyOptions = fs::copy_options::overwrite_existing 
                               | fs::copy_options::recursive;

        fs::copy(tmpDir + "/data/", installDir + "/data/", copyOptions);
        fs::copy(tmpDir + "/font/", installDir + "/font/", copyOptions);
        fs::copy(tmpDir + "/kd/", installDir + "/kn/", copyOptions);
        fs::copy(tmpDir + "/vsh/etc/", installDir + "/vsh/etc/", copyOptions);
        fs::copy(tmpDir + "/vsh/module/", installDir + "/vsh/nodule/", copyOptions);
        fs::copy(tmpDir + "/vsh/resource/", installDir + "/vsh/resource/", copyOptions);

        //Decrypt boot config files and insert systemctrl.prx
        updateBtCfg(installDir + "/kn/pspbtcnf.txt",{  { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" } });
        updateBtCfg(installDir + "/kn/pspbtcnf_game.txt",{  { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" } });
        updateBtCfg(installDir + "/kn/pspbtcnf_updater.txt",{  { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" } });

        writeFileWithEmbeddedReboot(installDir + "/kd/systemctrl.prx", tmpDir + "/PSARDUMPER/reboot.bin", 
                            tm271_systemctrl150, sizeof(tm271_systemctrl150), 0x678, false);

        writeFileWithEmbeddedReboot(installDir + "/kn/systemctrl.prx", installDir + "/PSARDUMPER/reboot.bin", 
                            tm271_systemctrl, sizeof(tm271_systemctrl), 0xa27c, true);

        fs::remove_all(tmpDir);
        fs::remove_all(installDir + "/PSARDUMPER");  
    }
    else if (version.compare("2.80") == 0) {
        string installDir = tmDir + "/280";
        extractFirmware(installDir, upDir, "280", true, true, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm280_payload, sizeof(tm280_payload));
        WriteFile((installDir + "/tmctrl280.prx").c_str(), (void*)tm280_tmctrl280, sizeof(tm280_tmctrl280));
    }
    else if (version.compare("3.40OE-A") == 0) {
        string installDir = tmDir + "/340OE";
        string tmpDir = installDir + "/tmp";

        extractFirmware(installDir, upDir, "150", false, true, verbose);

        //Decrypt and update boot config files
        updateBtCfg(installDir + "/kd/pspbtcnf_game.txt", {
                        { "/kd/uart4.prx\n", "/kd/uart4r.prx #/kd/uart4.prx\n" },
                        { "/kd/loadexec.prx\n", "/kd/syscon.prx #/kd/loadexec.prx\n" },
                        { "/kd/emc_sm.prx\n", "/kd/emc_smr.prx #/kd/emc_sm.prx\n" },
                        { "/kd/emc_ddr.prx\n", "/kd/emc_ddrr.prx #/kd/emc_ddr.prx\n" },
                        { "/kd/ge.prx\n", "/kd/emc_sm.prx #/kd/ge.prx\n" },
                        { "/kd/idstorage.prx\n", "/kd/idstorager.prx #/kd/idstorage.prx\n" },
                        { "/kd/syscon.prx\n", "/kd/emc_ddr.prx #/kd/syscon.prx\n" },
                        { "/kd/rtc.prx\n", "/kd/rtcr.prx #/kd/rtc.prx\n" },
                        { "/kd/display.prx\n", "/kd/ge.prx #/kd/display.prx\n" },
                        { "/kd/ctrl.prx\n", "/kd/idstorage.prx #/kd/ctrl.prx\n" },
                        { "/kd/impose.prx\n", "/kd/impose.prx\n/kd/rtc.prx #/kd/systemctrl.prx\n" }
                    });
        updateBtCfg(installDir + "/kd/pspbtcnf_updater.txt", {
                        { "/kd/uart4.prx\n", "/kd/uart4r.prx #/kd/uart4.prx\n" },
                        { "/kd/loadexec.prx\n", "/kd/syscon.prx #/kd/loadexec.prx\n" },
                        { "/kd/emc_sm.prx\n", "/kd/emc_smr.prx #/kd/emc_sm.prx\n" },
                        { "/kd/emc_ddr.prx\n", "/kd/emc_ddrr.prx #/kd/emc_ddr.prx\n" },
                        { "/kd/ge.prx\n", "/kd/emc_sm.prx #/kd/ge.prx\n" },
                        { "/kd/idstorage.prx\n", "/kd/idstorager.prx #/kd/idstorage.prx\n" },
                        { "/kd/syscon.prx\n", "/kd/emc_ddr.prx #/kd/syscon.prx\n" },
                        { "/kd/rtc.prx\n", "/kd/rtcr.prx #/kd/rtc.prx\n" },
                        { "/kd/display.prx\n", "/kd/ge.prx #/kd/display.prx\n" },
                        { "/kd/ctrl.prx\n", "/kd/idstorage.prx #/kd/ctrl.prx\n" },
                        { "/kd/vshbridge.prx\n", "/kd/vshbridge.prx\n/kd/rtc.prx #/kd/systemctrl.prx\n" }
                    });

        const auto copyOptions = fs::copy_options::overwrite_existing 
                               | fs::copy_options::recursive;

        fs::copy(installDir + "/kd/emc_ddr.prx", installDir + "/kd/emc_ddrr.prx", copyOptions);
        fs::copy(installDir + "/kd/emc_sm.prx", installDir + "/kd/emc_smr.prx", copyOptions);
        fs::copy(installDir + "/kd/idstorage.prx", installDir + "/kd/idstorager.prx", copyOptions);
        fs::copy(installDir + "/kd/rtc.prx", installDir + "/kd/rtcr.prx", copyOptions);
        fs::copy(installDir + "/kd/uart4.prx", installDir + "/kd/uart4r.prx", copyOptions);
        fs::copy(installDir + "/kd/syscon.prx", installDir + "/kd/emc_ddr.prx", copyOptions);
        fs::copy(installDir + "/kd/loadexec.prx", installDir + "/kd/syscon.prx", copyOptions);
        fs::copy(installDir + "/kd/ge.prx", installDir + "/kd/emc_sm.prx", copyOptions);
        fs::copy(installDir + "/kd/ctrl.prx", installDir + "/kd/idstorage.prx", copyOptions);
        fs::copy(installDir + "/kd/display.prx", installDir + "/kd/ge.prx", copyOptions);

        //Remove unused 1.50 firmware files
        fs::remove_all(installDir + "/data/cert/");
        fs::remove_all(installDir + "/vsh/etc/");
        fs::remove_all(installDir + "/vsh/resource/");
        fs::remove(installDir + "/kd/ctrl.prx");
        fs::remove(installDir + "/kd/display.prx");
        fs::remove(installDir + "/kd/loadexec.prx");
        fs::remove(installDir + "/kd/rtc.prx");
        fs::remove(installDir + "/kd/mebooter_umdvideo.prx");
        fs::remove(installDir + "/kd/reboot.prx");

        const char *modules[13] = { "auth_plugin", "game_plugin", "impose_plugin", "msgdialog_plugin",
                                    "msvideo_plugin", "music_plugin", "opening_plugin", "photo_plugin",
                                    "sysconf_plugin", "update_plugin", "video_plugin", "paf", "vshmain" };

        for (int i = 0; i < 13; i++) {
            fs::remove(installDir + "/vsh/module/" + modules[i] + ".prx");
        }

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm340_payload, sizeof(tm340_payload));
        WriteFile((installDir + "/tmctrl150.prx").c_str(), (void*)tm340_tmctrl150, sizeof(tm340_tmctrl150));
        WriteFile((installDir + "/tmctrl340.prx").c_str(), (void*)tm340_tmctrl340, sizeof(tm340_tmctrl340));
        WriteFile((installDir + "/kd/recovery.prx").c_str(), (void*)tm340_recovery, sizeof(tm340_recovery));
        WriteFile((installDir + "/kd/uart4.prx").c_str(), (void*)tm340_lcpatcher, sizeof(tm340_lcpatcher));

        //Setup 340

        extractFirmware(tmpDir, upDir, "340", false, false, verbose);

        fs::create_directory(installDir + "/data/cert");

        fs::copy(tmpDir + "/data/cert/CA_LIST.cer", installDir + "/data/cert", copyOptions);
        fs::copy(tmpDir + "/font/", installDir + "/font/", copyOptions);
        fs::copy(tmpDir + "/gps/", installDir + "/gps/", copyOptions);
        fs::copy(tmpDir + "/kd/", installDir + "/kn/", copyOptions);
        fs::copy(tmpDir + "/net/", installDir + "/net/", copyOptions);
        fs::copy(tmpDir + "/vsh/etc/", installDir + "/vsh/etc/", copyOptions);
        fs::copy(tmpDir + "/vsh/module/", installDir + "/vsh/nodule/", copyOptions);
        fs::copy(tmpDir + "/vsh/resource/", installDir + "/vsh/resource/", copyOptions);

        //Decrypt and update boot config files
        updateBtCfg(installDir + "/kn/pspbtcnf.txt", {
                        { ".prx [a-f0-9]+\n", ".prx\n" },
                        { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" },
                        { "/kd/vshbridge.prx", "/kd/vshbridge.prx\n/kd/vshctrl.prx" }
                    });
        updateBtCfg(installDir + "/kn/pspbtcnf_game.txt", {
                        { ".prx [a-f0-9]+\n", ".prx\n" },
                        { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" }
                    });
        updateBtCfg(installDir + "/kn/pspbtcnf_licensegame.txt", {
                        { ".prx [a-f0-9]+\n", ".prx\n" },
                        { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" }
                    });
        updateBtCfg(installDir + "/kn/pspbtcnf_pops.txt", {
                        { ".prx [a-f0-9]+\n", ".prx\n" },
                        { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" },
                        { "\\$/kd/idmanager.prx", "#$/kd/idmanager.prx\n/kd/idcanager.prx" },
                        { "/kd/popsman.prx", "/kd/popsman.prx\n/kd/popcorn.prx" }
                    });
        updateBtCfg(installDir + "/kn/pspbtcnf_updater.txt", {
                        { ".prx [a-f0-9]+\n", ".prx\n" },
                        { "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx" }
                    });

        WriteFile((installDir + "/kn/idcanager.prx").c_str(), (void*)tm340_idcanager, sizeof(tm340_idcanager));
        WriteFile((installDir + "/kn/popcorn.prx").c_str(), (void*)tm340_popcorn, sizeof(tm340_popcorn));
        WriteFile((installDir + "/kn/reboot150.prx").c_str(), (void*)tm340_reboot150, sizeof(tm340_reboot150));
        WriteFile((installDir + "/kn/systemctrl.prx").c_str(), (void*)tm340_systemctrl, sizeof(tm340_systemctrl));
        WriteFile((installDir + "/kn/vshctrl.prx").c_str(), (void*)tm340_vshctrl, sizeof(tm340_vshctrl));

        writeFileWithEmbeddedReboot(installDir + "/kd/rtc.prx", tmpDir + "/PSARDUMPER/reboot.bin", 
                            tm340_systemctrl150, sizeof(tm340_systemctrl150), 0x8a0, false);

        writeFileWithEmbeddedReboot(installDir + "/kn/reboot150.prx", installDir + "/PSARDUMPER/reboot.bin", 
                            tm340_reboot150, sizeof(tm340_reboot150), 0x380, true);

        fs::remove_all(tmpDir);
        fs::remove_all(installDir + "/PSARDUMPER");      
    }
    else {
        cerr << "Unsupported version " << version << "!" << endl;
        help(argv[0]);
    }

    return 0;
}
