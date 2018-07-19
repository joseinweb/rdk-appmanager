/**
 * Provides info about current device
 * based on data from servicemanager, authService
 */

px.import({scene: "px:scene.1.js", http: "http"})
.then(function (imports) {

  var scene = imports.scene;
  var http = imports.http;

  // some of the possible names to be used as params for getDeviceInfo, getRFCConfig, getCustomInfo
  var DEVICEINFO_NAME_ESTB_MAC = "estb_mac";
  var RFC_NAME_TRAFFICSHAPING_ENABLE = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TrafficShaping.Enable";
  var RFC_NAME_TRAFFICSHAPING_URL = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TrafficShaping.TSserverUrl";

  function getModel() {
    return new Promise(function (resolve, reject) {
      try {
        var s = scene.getService("systemService");
        s.setApiVersionNumber(8);
        var callStr = s.callMethod("getXconfParams");
        resolve(JSON.parse(callStr)["xconfParams"][0].model.trim());
      } catch (e) {
        reject(new Error("getModel failed: "+e));
      }
    });
  }

  function getStbVersion() {
    return new Promise(function (resolve, reject) {
      try {
        var s = scene.getService("systemService");
        s.setApiVersionNumber(4);
        var callStr = s.callMethod("getSystemVersions");
        resolve(JSON.parse(callStr).stbVersion.trim());
      } catch (e) {
        reject(new Error("getStbVersion failed: "+e));
      }
    });
  }

  function isWifiDevice() {
    return new Promise(function (resolve) {
      try {
        var s = scene.getService("org.rdk.wifiManager");
        resolve(s.getName() === "org.rdk.wifiManager");
      } catch (e) {
        resolve(false);
      }
    });
  }

  function isConnectedLNF() {
    return new Promise(function (resolve) {
      try {
        var s = scene.getService("org.rdk.wifiManager");
        var callStr = s.callMethod("isConnectedLNF");
        resolve(true === JSON.parse(callStr).connectedLNF);
      } catch (e) {
        resolve(false);
      }
    });
  }

  function getDeviceInfo(name) {
    return new Promise(function (resolve, reject) {
      try {
        var s = scene.getService("systemService");
        var callStr = s.callMethod("getDeviceInfo", JSON.stringify({"params": [name]}));
        var info = JSON.parse(callStr)[name].trim();
        if (info === "Acquiring") {
          reject(new Error("getDeviceInfo failed: not ready"));
        } else {
          resolve(info);
        }
      } catch (e) {
        reject(new Error("getDeviceInfo failed: "+e));
      }
    });
  }

  function getDeviceId() {
    return new Promise(function (resolve, reject) {
      var req = http.request({
        host: "127.0.0.1",
        port: 50050,
        path: "/authService/getDeviceId",
        method: "POST"
      }, function (res) {
        var str = '';
        res.on('data', function (chunk) {
          str += chunk;
        });
        res.on('end', function () {
          try {
            var obj = JSON.parse(str);
            resolve({deviceId: obj.deviceId, partnerId: obj.partnerId});
          } catch (e) {
            reject(new Error("getDeviceId parse failed: "+e));
          }
        });
      });
      req.on('error', function(e) {
        reject(new Error("getDeviceId failed: "+e));
      });
      req.setTimeout(60000, function () {
        req.abort();
        reject(new Error("getDeviceId timed out"));
      });
      req.end();
    });
  }

  function getRFCConfig(names) {
    return new Promise(function (resolve, reject) {
      try {
        var s = scene.getService("systemService");
        var callStr = s.callMethod("getRFCConfig", JSON.stringify({"params": names}));
        var obj = JSON.parse(callStr)["values"][0];
        if (obj !== null && typeof obj === 'object') {
          resolve(obj);
        } else {
          reject(new Error("getRFCConfig failed: bad output"));
        }
      } catch (e) {
        reject(new Error("getRFCConfig failed: "+e));
      }
    });
  }

  function getCustomInfo(names) {
    var promises = [];
    var result = {};
    if (names.indexOf("model") >= 0) {
      promises.push(getModel()
      .then(function (val) {
        result.model = val;
      }));
    }
    if (names.indexOf("stbVersion") >= 0) {
      promises.push(getStbVersion()
      .then(function (val) {
        result.stbVersion = val;
      }));
    }
    if (names.indexOf("isWifiDevice") >= 0 || names.indexOf("isConnectedLNF") >= 0) {
      promises.push(isWifiDevice()
      .then(function (val) {
        if (names.indexOf("isWifiDevice") >= 0) {
          result.isWifiDevice = val;
        }
        if (names.indexOf("isConnectedLNF") >= 0) {
          result.isConnectedLNF = false;
          if (val === true) {
            return isConnectedLNF()
            .then(function (val) {
              result.isConnectedLNF = val;
            });
          }
        }
      }));
    }
    if (names.indexOf(DEVICEINFO_NAME_ESTB_MAC) >= 0) {
      promises.push(getDeviceInfo(DEVICEINFO_NAME_ESTB_MAC)
      .then(function (val) {
        result[DEVICEINFO_NAME_ESTB_MAC] = val;
      }));
    }
    if (names.indexOf("deviceId") >= 0 || names.indexOf("partnerId") >= 0) {
      promises.push(getDeviceId()
      .then(function (val) {
        if (names.indexOf("deviceId") >= 0) {
          result.deviceId = val.deviceId;
        }
        if (names.indexOf("partnerId") >= 0) {
          result.partnerId = val.partnerId;
        }
      }));
    }
    if (names.indexOf(RFC_NAME_TRAFFICSHAPING_URL) >= 0) {
      promises.push(getRFCConfig([
        RFC_NAME_TRAFFICSHAPING_ENABLE,
        RFC_NAME_TRAFFICSHAPING_URL])
      .then(function (val) {
        var enable = val[RFC_NAME_TRAFFICSHAPING_ENABLE];
        var url = val[RFC_NAME_TRAFFICSHAPING_URL];
        if (enable !== "true" || typeof url !== 'string' || !url) {
          throw new Error("disabled in RFC");
        }
        result[RFC_NAME_TRAFFICSHAPING_URL] = url;
      }));
    }
    return Promise.all(promises)
    .then(function () {
      return result;
    });
  }

  module.exports.getModel = getModel;
  module.exports.getStbVersion = getStbVersion;
  module.exports.isWifiDevice = isWifiDevice;
  module.exports.isConnectedLNF = isConnectedLNF;
  module.exports.getDeviceInfo = getDeviceInfo;
  module.exports.getDeviceId = getDeviceId;
  module.exports.getRFCConfig = getRFCConfig;
  module.exports.DEVICEINFO_NAME_ESTB_MAC = DEVICEINFO_NAME_ESTB_MAC;
  module.exports.RFC_NAME_TRAFFICSHAPING_ENABLE = RFC_NAME_TRAFFICSHAPING_ENABLE;
  module.exports.RFC_NAME_TRAFFICSHAPING_URL = RFC_NAME_TRAFFICSHAPING_URL;
  module.exports.getCustomInfo = getCustomInfo;

})
.catch(function (err) {
  console.error("Import failed: " + err);
});
