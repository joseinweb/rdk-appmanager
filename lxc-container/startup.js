px.import({ scene: 'px:scene.1.js', keys: 'px:tools.keys.js',
  http: 'http', https: 'https', url: 'url', deviceInfo: 'deviceInfo.js',
  Optimus: 'optimus.js' }).then( function importsAreReady(imports)
{
  var scene = imports.scene;
  var keys  = imports.keys;
  var optimus = imports.Optimus;
  var root  = imports.scene.root;
  var deviceInfo = imports.deviceInfo;
  var http = imports.http;
  var https = imports.https;
  var url = imports.url;

  var startupRoot = null;

  //this is needed to prevent black background
  module.exports.wantsClearscreen = function()
  {
    return false;
  };

  var startupOverride = px.appQueryParams.startupOverride;
  
  if (!startupOverride) {
  
    var id = "1001";
    var launchParams = { "cmd":"wpe"};

    scene.addServiceProvider(function(serviceName, serviceCtx){                   
      if (serviceName === ".optimus")
      {
        return optimus;
      }
      if (serviceName === ".startupTest")
      {
        return {testMethod:function(){console.log("test function");}};
      }
      return "allow";
    });
    
    function createApp() {
      optimus.setScene(scene);
      optimus.createApplication({
        id:id, priority:1, x:0, y:0, w:scene.getWidth(), h:scene.getHeight(),
        cx:0, cy:0, sx:1.0, sy:1.0, r:0, a:1, interactive:true,
        painting:true, clip:false, mask:false, draw:true,
        launchParams:launchParams
      });
      optimus.getApplicationById(id).setFocus(true);
    }

    deviceInfo.getCustomInfo([
      "model", "stbVersion", deviceInfo.DEVICEINFO_NAME_ESTB_MAC,
      "isWifiDevice", "isConnectedLNF",
      "deviceId", "partnerId",
      deviceInfo.RFC_NAME_TRAFFICSHAPING_URL
    ])
    .then(function (info) {
      return new Promise(function (resolve, reject) {
        var tsUrl = info[deviceInfo.RFC_NAME_TRAFFICSHAPING_URL];
        delete info[deviceInfo.RFC_NAME_TRAFFICSHAPING_URL];
        var tsParams = JSON.stringify({"deviceCaps":info});
        console.log("traffic shaping url="+tsUrl+" params="+tsParams);
        var options = Object.assign({}, url.parse(tsUrl));
        options.method = 'POST';
        options.headers = { 'Content-Type': 'application/json' };
        var moduleFn = tsUrl.indexOf("https") === 0 ? https.request : http.request;
        var req = moduleFn(options, function (res) {
          if (res.statusCode === 302 && res.headers.location) {
            resolve(res.headers.location);
          } else {
            reject(new Error("not redirected"));
          }
        });
        req.setTimeout(10000, function () {
          req.abort();
          reject(new Error("timed out"));
        });
        req.on("error", function () {
          reject(new Error("http error"));
        });
        req.write(tsParams);
        req.end();
      });
    })
    .then(function (url) {
      console.log("traffic shaping app: " + url);
      launchParams = { "cmd":"spark", "uri":url };
      createApp();
    }, function (e) {
      console.warn("traffic shaping unavailable: " + e);
      createApp();
    });
  } else {
    var handleEasterEgg = true;
    var easterEggs = {};
    var allEggs;
    var eggSearchPattern = "";
    var keydownTimestamp = 0;
    var easterEggTrigger = 1; // milliseconds
    if (handleEasterEgg){
      setupEasterEgg();
    }

    function reloadStartupOverride(){
      console.log("reloading url");
      startupRoot.url = startupOverride;
    }

    function EasterEgg(inPattern, inType, inAction){
      this.pattern = inPattern;
      this.type = inType;
      this.action = inAction;
    }

    function setupEasterEgg(){
      easterEggs[keys.DOWN +"-"+ keys.DOWN +"-"+ keys.DOWN +"-"+ keys.SEVEN +"-"+ keys.THREE] = new EasterEgg("DOWN-DOWN-DOWN-SEVEN-THREE", "2", reloadStartupOverride);

      allEggs = Object.keys(easterEggs);
    }

    function processEasterEgg(inKeyCode, inFlags){
      if (eggSearchPattern == "") {
        eggSearchPattern = eggSearchPattern + inKeyCode;
      }
      else{
        eggSearchPattern = eggSearchPattern + "-" + inKeyCode;
      }
      //console.log("Egg pattern string" + eggSearchPattern);
      if (easterEggs[eggSearchPattern] !== undefined) {
        //console.log("Found easter egg" + eggSearchPattern);
        easterEggs[eggSearchPattern].action.call();
        eggSearchPattern = "";
      }
      else {
        var matchFound = false;
        for(var key in allEggs) {
          var regexp = "^" + eggSearchPattern;
          //console.log("allEggs[key]" + allEggs[key] + " " + regexp);
          if (allEggs[key].match(regexp)) {
            matchFound = true;
            break;
          }
        }
        if (!matchFound) {
          eggSearchPattern = "";
        }
      }
    }

    //below is added to get focus back to demoRoot object on ctrl+L
    root.on("onPreKeyDown",function (e) {
      var code = e.keyCode; var flags = e.flags;
      //console.log("startup.js onPreKeyDown keycode: " + e.keyCode);
      if (keydownTimestamp === 0){
        keydownTimestamp = scene.clock();
      }
    });

    root.on("onPreKeyUp",function (e) {
      //console.log("key hold value: " + (scene.clock() - keydownTimestamp));
      if (handleEasterEgg &&  ((scene.clock() - keydownTimestamp) > easterEggTrigger)) {
        processEasterEgg(e.keyCode, e.flags);
      }else {
        eggSearchPattern = "";
      }
      keydownTimestamp = 0;
    });
    startupRoot = scene.create({t:"scene", parent:root, url:startupOverride, w:scene.getWidth(), h:scene.getHeight(), focus:true});
  }

}).catch( function importFailed(err){
  console.error("Import failed for startup.js: " + err);
});
