#include "web_portal.h"
#include <Update.h>

// Static references set during setup, used by route handlers
static WebServer *pServer = nullptr;
static AppConfig *pConfig = nullptr;
static ConfigSavedCallback onSaveCallback = nullptr;

static const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Choose Your Own GPT</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px;max-width:800px;margin:0 auto}
h1{color:#e94560;margin:0 0 16px;font-size:24px}
h2{color:#fff;background:#e94560;padding:8px 12px;border-radius:4px 4px 0 0;margin:20px 0 0;font-size:16px}
.s{background:#16213e;padding:16px;border-radius:0 0 8px 8px;margin-bottom:4px}
label{display:block;margin:10px 0 4px;font-size:13px;color:#8899aa}
input,textarea,select{width:100%;padding:8px;background:#0f3460;border:1px solid #1a4080;color:#fff;border-radius:4px;font-size:14px;font-family:inherit}
input:focus,textarea:focus{outline:none;border-color:#e94560}
textarea{min-height:100px;resize:vertical}
.btn{display:inline-block;background:#e94560;color:#fff;border:none;padding:10px 24px;border-radius:4px;cursor:pointer;font-size:14px;margin:8px 8px 0 0}
.btn:hover{background:#d13b55}
.btn-r{background:#444}
.btn-r:hover{background:#666}
.bar{position:fixed;top:0;left:0;right:0;padding:10px 16px;z-index:100;display:none;font-size:14px;text-align:center}
.bar.ok{display:block;background:#1b5e20}
.bar.err{display:block;background:#b71c1c}
.note{font-size:12px;color:#667;margin-top:4px}
</style></head><body>
<div id="bar" class="bar"></div>
<h1>&#9881; Choose Your Own GPT</h1>

<h2>General Settings</h2>
<div class="s">
<label>OpenAI API Key</label>
<input type="password" id="apiKey" placeholder="sk-...">
<div class="note">Stored on the device only. Never shared externally.</div>
<label>OpenAI Model</label>
<input type="text" id="model" placeholder="gpt-4o">
<label style="display:flex;align-items:center;gap:8px;margin-top:12px;cursor:pointer">
<input type="checkbox" id="hyphenate" style="width:auto">
<span>Hyphenate long words when wrapping</span>
</label>
</div>

<h2>Character Names (Small Dial)</h2>
<div class="s" id="nd"></div>

<h2>Adventure Destinations (Large Dial)</h2>
<div class="s" id="ad"></div>

<h2>Story Prompts</h2>
<div class="s">
<label>Story Prompt (prepended before character names)</label>
<textarea id="sp"></textarea>
<label>Instructions (appended after adventure destination)</label>
<textarea id="ins"></textarea>
<label>Surprise Ending Prompt</label>
<textarea id="se"></textarea>
<label>Blue Button - Continue</label>
<input type="text" id="bc">
<label>Blue Button - Final Chapter</label>
<input type="text" id="bf">
<label>Yellow Button - Continue</label>
<input type="text" id="yc">
<label>Yellow Button - Final Chapter</label>
<input type="text" id="yf">
</div>

<div style="margin:20px 0">
<button class="btn" onclick="saveAll()">&#128190; Save Configuration</button>
<button class="btn btn-r" onclick="resetDefaults()">&#128260; Reset to Defaults</button>
</div>

<h2>Import / Export</h2>
<div class="s">
<button class="btn btn-r" onclick="exportCfg()">&#11015; Export Settings JSON</button>
<label style="margin-top:12px">Import settings from JSON file</label>
<input type="file" id="imp" accept=".json">
<button class="btn" onclick="importCfg()">&#11014; Import Settings JSON</button>
</div>

<h2>Firmware Update (OTA)</h2>
<div class="s">
<label>Select firmware .bin file</label>
<input type="file" id="fw" accept=".bin">
<button class="btn" onclick="doOta()">&#11014; Upload Firmware</button>
<div id="otaSt" class="note" style="margin-top:8px"></div>
</div>

<script>
function esc(s){var d=document.createElement('div');d.textContent=s;return d.innerHTML.replace(/"/g,'&quot;');}
function $(id){return document.getElementById(id);}

async function load(){
try{
var r=await fetch('/api/config');
var c=await r.json();
$('apiKey').value=c.apiKey||'';
$('model').value=c.model||'';
$('hyphenate').checked=!!c.hyphenate;
var nd=$('nd');nd.innerHTML='';
(c.names||[]).forEach(function(n,i){nd.innerHTML+='<label>Position '+i+'</label><input type="text" class="ni" value="'+esc(n)+'">';});
var ad=$('ad');ad.innerHTML='';
(c.adventures||[]).forEach(function(a,i){ad.innerHTML+='<label>Position '+i+'</label><input type="text" class="ai" value="'+esc(a)+'">';});
var p=c.prompts||{};
$('sp').value=p.storyPrompt||'';
$('ins').value=p.instructions||'';
$('se').value=p.surpriseEnding||'';
$('bc').value=p.blueContinue||'';
$('bf').value=p.blueComplete||'';
$('yc').value=p.yellowContinue||'';
$('yf').value=p.yellowComplete||'';
}catch(e){msg('Failed to load: '+e,false);}
}

async function saveAll(){
var data={
apiKey:$('apiKey').value,
model:$('model').value,
hyphenate:$('hyphenate').checked,
names:[].map.call(document.querySelectorAll('.ni'),function(i){return i.value;}),
adventures:[].map.call(document.querySelectorAll('.ai'),function(i){return i.value;}),
prompts:{
storyPrompt:$('sp').value,
instructions:$('ins').value,
surpriseEnding:$('se').value,
blueContinue:$('bc').value,
blueComplete:$('bf').value,
yellowContinue:$('yc').value,
yellowComplete:$('yf').value
}};
try{
var r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
if(r.ok)msg('Configuration saved successfully!',true);
else msg('Save failed: '+r.statusText,false);
}catch(e){msg('Save failed: '+e,false);}
}

async function resetDefaults(){
if(!confirm('Reset all settings to factory defaults? Your API key will be cleared.'))return;
try{
var r=await fetch('/api/config/reset',{method:'POST'});
if(r.ok){msg('Reset to defaults. Reloading...',true);setTimeout(load,500);}
else msg('Reset failed',false);
}catch(e){msg('Reset failed: '+e,false);}
}

function msg(t,ok){var b=$('bar');b.textContent=t;b.className='bar '+(ok?'ok':'err');setTimeout(function(){b.className='bar';},3000);}

function exportCfg(){
fetch('/api/config').then(function(r){return r.text();}).then(function(t){
var b=new Blob([t],{type:'application/json'});
var a=document.createElement('a');a.href=URL.createObjectURL(b);a.download='cyogpt-config.json';a.click();
msg('Settings exported!',true);
}).catch(function(e){msg('Export failed: '+e,false);});}

function importCfg(){
var f=$('imp').files[0];
if(!f){msg('Please select a JSON file first',false);return;}
var rd=new FileReader();
rd.onload=async function(ev){
try{
var r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:ev.target.result});
if(r.ok){msg('Settings imported! Reloading...',true);setTimeout(load,600);}
else msg('Import failed: '+r.statusText,false);
}catch(err){msg('Import failed: '+err,false);}
};
rd.readAsText(f);}

function doOta(){
var f=$('fw').files[0];
if(!f){msg('Please select a .bin firmware file',false);return;}
if(!confirm('Upload '+f.name+' as firmware?\nThe device will reboot after a successful update.'))return;
$('otaSt').textContent='Uploading \u2013 please wait\u2026';
msg('Uploading firmware, please wait\u2026',true);
var fd=new FormData();fd.append('firmware',f,f.name);
fetch('/ota/update',{method:'POST',body:fd}).then(function(r){
var ok=r.ok;return r.text().then(function(t){return {ok:ok,t:t};});
}).then(function(res){
$('otaSt').textContent=res.t;msg(res.t,res.ok);
}).catch(function(e){$('otaSt').textContent='Error: '+e;msg('Upload failed: '+e,false);});}

load();
</script>
</body></html>
)rawliteral";

// Shared state for the OTA upload handler and completion handler.
// Update.abort() internally clears the error flag, so we track failures
// separately to ensure the completion handler can report them correctly.
static bool   s_otaFailed   = false;
static String s_otaErrorMsg;

// Milliseconds to wait after sending the HTTP response before rebooting.
// flush() pushes data into the TCP send buffer; the delay gives the TCP
// stack time to finish transmission before the network interface goes down.
static const int OTA_REBOOT_DELAY_MS = 200;

static void handleConfigPage() {
  pServer->send(200, "text/html", CONFIG_HTML);
}

static void handleGetConfig() {
  JsonDocument doc;
  configToJson(*pConfig, doc);
  String json;
  serializeJson(doc, json);
  pServer->send(200, "application/json", json);
}

static void handleSaveConfig() {
  if (!pServer->hasArg("plain")) {
    pServer->send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, pServer->arg("plain"));
  if (err) {
    String errMsg = "{\"error\":\"Invalid JSON: ";
    errMsg += err.c_str();
    errMsg += "\"}";
    pServer->send(400, "application/json", errMsg);
    return;
  }

  configFromJson(*pConfig, doc);
  configSave(*pConfig);
  pServer->send(200, "application/json", "{\"status\":\"ok\"}");

  if (onSaveCallback) onSaveCallback();
}

static void handleResetConfig() {
  configReset(*pConfig);
  pServer->send(200, "application/json", "{\"status\":\"ok\"}");

  if (onSaveCallback) onSaveCallback();
}

void webPortalSetup(WebServer &server, AppConfig &cfg, ConfigSavedCallback onSave) {
  pServer = &server;
  pConfig = &cfg;
  onSaveCallback = onSave;

  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  server.on("/api/config/reset", HTTP_POST, handleResetConfig);

  // OTA firmware update endpoint
  server.on(
    "/ota/update", HTTP_POST,
    // Completion handler – runs after the upload finishes
    []() {
      bool ok = !s_otaFailed && !Update.hasError();
      pServer->sendHeader("Connection", "close");
      if (ok) {
        pServer->send(200, "text/plain", "Update OK \u2013 rebooting");
        pServer->client().flush();  // push response bytes into the TCP send buffer
        delay(OTA_REBOOT_DELAY_MS);
        ESP.restart();
      } else {
        String errMsg = "Update FAILED: ";
        errMsg += s_otaFailed ? s_otaErrorMsg : Update.errorString();
        pServer->send(500, "text/plain", errMsg);
      }
    },
    // Upload handler – called for each chunk of the incoming multipart body
    []() {
      HTTPUpload &up = pServer->upload();
      if (up.status == UPLOAD_FILE_START) {
        s_otaFailed   = false;
        s_otaErrorMsg = "";
        Serial.printf("OTA: start %s\n", up.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          s_otaErrorMsg = Update.errorString();
          s_otaFailed   = true;
          Serial.printf("OTA: begin failed: %s\n", s_otaErrorMsg.c_str());
        }
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (Update.isRunning()) {
          if (Update.write(up.buf, up.currentSize) != up.currentSize) {
            s_otaErrorMsg = Update.errorString();
            s_otaFailed   = true;
            Serial.printf("OTA: write failed at %u bytes (chunk %u): %s\n",
                          up.totalSize, up.currentSize, s_otaErrorMsg.c_str());
            Update.abort();  // abort stops further writes; clears Update error state
          }
        }
      } else if (up.status == UPLOAD_FILE_END) {
        if (Update.isRunning()) {
          if (Update.end(true)) {
            Serial.printf("OTA: success, %u bytes\n", up.totalSize);
          } else {
            s_otaErrorMsg = Update.errorString();
            s_otaFailed   = true;
            Serial.printf("OTA: end failed: %s\n", s_otaErrorMsg.c_str());
          }
        }
      }
    }
  );
}
