yaml = require('js-yaml');
fs   = require('fs');
const net = require('net');
const port = 7070;
const host = '127.0.0.1';
 
console.log("ScopeToSky - command line version");
console.log("Copyright Mel Bartel https://www.bbastrodesigns.com/scopeToSky.html");
console.log("Adapted to node.js by François Desvallées https://github.com/fdesvallees")

// Import libraries
MLB = {};
MLB.underscoreLib = require ('./lib/underscore/underscore-min.js')
MLB.sharedLib = require('./lib/sharedLib.js');
MLB.coordLib = require('./lib/coordLib.js');
MLB.encoderLib = require('./lib/encoderLib.js');
MLB.calcLib = require('./lib/calcLib.js');
MLB.scopeToSky = require('./lib/scopeToSky.js');

// Start TCP server
const server = net.createServer();
server.listen(port, host, () => {
    console.log('TCP Server is running on port ' + port + '.');
});

let sockets = [];

// Listen to client, perform computation 
server.on('connection', function(sock) {
    console.log('CONNECTED: ' + sock.remoteAddress + ':' + sock.remotePort);
    sockets.push(sock);

    sock.on('data', function(data) {

      var s = data.toString().replace(/(\r\n|\n|\r)/gm,""); // strip newlines
      var config = JSON.parse(s);

      if (config != null)
      {
        if (config.command === 'computeScope')
        {
          MLB.scopeToSky.setInitialValues(config);
          MLB.scopeToSky.processDateTime(config);
          MLB.scopeToSky.computeScope(config);          
        }
        if (config.command === 'quit')
        {
          process.exit();
        }

        // Write the result back to sender
        sockets.forEach(function(sock, index, array) {
          sock.write(JSON.stringify(MLB.scopeToSky.scopeToSkyState)+ '\n');
        });
      }
    });

    // Add a 'close' event handler to this instance of socket
    sock.on('close', function(data) {
        let index = sockets.findIndex(function(o) {
            return o.remoteAddress === sock.remoteAddress && o.remotePort === sock.remotePort;
        })
        if (index !== -1) sockets.splice(index, 1);
        console.log('CLOSED: ' + sock.remoteAddress + ' ' + sock.remotePort);
    });
});

