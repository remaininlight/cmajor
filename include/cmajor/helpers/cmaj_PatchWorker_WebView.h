//
//     ,ad888ba,                              88
//    d8"'    "8b
//   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
//   Y8,           88    88    88  88     88  88
//    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2024 Cmajor Software Ltd
//     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
//                                           ,88
//                                        888P"
//
//  The Cmajor project is subject to commercial or open-source licensing.
//  You may use it under the terms of the GPLv3 (see www.gnu.org/licenses), or
//  visit https://cmajor.dev to learn about our commercial licence options.
//
//  CMAJOR IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
//  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
//  DISCLAIMED.

#pragma once

#include "cmaj_Patch.h"
#include "../../choc/javascript/choc_javascript.h"
#include "../../choc/gui/choc_WebView.h"
#include "../../choc/text/choc_MIMETypes.h"


namespace cmaj
{

/// When you create a Patch object, you need to set its createContextForPatchWorker
/// property so that it knows what kind of javascript context to create to run
/// any patch workers that may be needed. This function sets up a hidden WebView
/// for that purpose, and gives it the appropriate library functions that it needs.
inline void enableWebViewPatchWorker (Patch& p)
{
    struct Worker : Patch::WorkerContext
    {
        Worker (Patch& p) : patch (p) {}
        ~Worker() override {}

        void initialise (std::function<void(const choc::value::ValueView&)> sendMessageToPatch,
                         std::function<void(const std::string&)> reportError) override
        {
            choc::ui::WebView::Options opts;
            opts.enableDebugMode = true;
            opts.fetchResource = [this] (const std::string& path) { return fetchResource (path); };
            webview = std::make_unique<choc::ui::WebView> (opts);

            webview->bind ("_cmaj_console_log", [] (const choc::value::ValueView& args) mutable -> choc::value::Value
            {
                if (args.isArray() && args.size() != 0)
                {
                    auto level = choc::javascript::LoggingLevel::log;

                    if (args.size() > 1)
                    {
                        switch (args[1].getWithDefault<int> (1))
                        {
                            case 0: level = choc::javascript::LoggingLevel::log; break;
                            case 1: level = choc::javascript::LoggingLevel::info; break;
                            case 2: level = choc::javascript::LoggingLevel::warn; break;
                            case 3: level = choc::javascript::LoggingLevel::error; break;
                            case 4: level = choc::javascript::LoggingLevel::debug; break;
                        }
                    }

                    consoleLog (args[0], level);
                }

                return {};
            });

            webview->bind ("cmaj_sendMessageToServer", [send = std::move (sendMessageToPatch)] (const choc::value::ValueView& args) -> choc::value::Value
            {
                if (args.isArray() && args.size() != 0)
                    send (args[0]);

                return {};
            });

            webview->bind ("cmaj_reportError", [reportError = std::move (reportError)] (const choc::value::ValueView& args) -> choc::value::Value
            {
                if (args.isArray() && args.size() != 0)
                    reportError (args[0].toString());

                return {};
            });

            webview->bind ("_internalReadResourceAsAudioData", [&p = patch] (const choc::value::ValueView& args) -> choc::value::Value
            {
                try
                {
                    if (args.isArray() && args.size() != 0)
                    {
                        if (auto path = args[0].toString(); ! path.empty())
                        {
                            choc::value::Value annotation;

                            if (args.size() > 1)
                                annotation = args[1];

                            if (auto manifest = p.getManifest())
                                return readManifestResourceAsAudioData (*manifest, path, annotation);
                        }
                    }
                }
                catch (...)
                {}

                return {};
            });

            webview->navigate ({});
        }

        static void consoleLog (const choc::value::ValueView& content, choc::javascript::LoggingLevel level)
        {
            auto message = content.isString() ? content.toString() : choc::json::toString (content);

            if (level == choc::javascript::LoggingLevel::debug)
                std::cerr << message << std::endl;
            else
                std::cout << message << std::endl;
        }

        void sendMessage (const std::string& msg, std::function<void(const std::string&)> reportError) override
        {
            webview->evaluateJavascript ("window.currentView?.deliverMessageFromServer(" + msg + ");",
                                         [reportError = std::move (reportError)] (const std::string& error, const choc::value::ValueView&)
            {
                if (! error.empty())
                    reportError (error);
            });
        }

        std::optional<choc::ui::WebView::Options::Resource> fetchResource (const std::string& path)
        {
            if (auto manifest = patch.getManifest())
            {
                if (path == "/")
                    return choc::ui::WebView::Options::Resource (getHTML (*manifest), "text/html");

                if (auto moduleText = readJavascriptResource (path, manifest))
                    return choc::ui::WebView::Options::Resource (*moduleText, choc::web::getMIMETypeFromFilename (path, "application/octet-stream"));
            }

            return {};
        }

        static std::string getHTML (const PatchManifest& manifest)
        {
            auto patchWorkerPath = manifest.patchWorker;

            if (! choc::text::startsWith (patchWorkerPath, "/"))
                patchWorkerPath = "/" + patchWorkerPath;

            return choc::text::replace (R"(
<!DOCTYPE html>
<html></html>

<script type="module">

import { PatchConnection } from "./cmaj_api/cmaj-patch-connection.js"

class WorkerPatchConnection  extends PatchConnection
{
    constructor()
    {
        super();
        this.manifest = MANIFEST;
        window.currentView = this;
    }

    getResourceAddress (path)
    {
        return path.startsWith ("/") ? path : ("/" + path);
    }

    sendMessageToServer (message)
    {
        cmaj_sendMessageToServer (message);
    }

    async readResource (path)
    {
        return fetch (path);
    }

    async readResourceAsAudioData (path)
    {
        return _internalReadResourceAsAudioData (path);
    }
}

window.console.log   =  function() { for (let a of arguments) _cmaj_console_log (a, 0); };
window.console.info  =  function() { for (let a of arguments) _cmaj_console_log (a, 1); };
window.console.warn  =  function() { for (let a of arguments) _cmaj_console_log (a, 2); };
window.console.error =  function() { for (let a of arguments) _cmaj_console_log (a, 3); };
window.console.debug =  function() { for (let a of arguments) _cmaj_console_log (a, 4); };

try
{
    const connection = new WorkerPatchConnection();

    const workerModule = await import (WORKER_MODULE);
    await workerModule.default (connection);
}
catch (e)
{
    window.cmaj_reportError (e.toString());
}

</script>
)",
            "MANIFEST", choc::json::toString (manifest.manifest),
            "WORKER_MODULE", choc::json::getEscapedQuotedString (patchWorkerPath));
        }

        Patch& patch;
        std::unique_ptr<choc::ui::WebView> webview;
    };

    p.createContextForPatchWorker = [&p]
    {
        return std::make_unique<Worker> (p);
    };
}

} // namespace cmaj
