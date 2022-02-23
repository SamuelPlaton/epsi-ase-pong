import { WebSocketServer } from 'ws';
import SerialPort from "serialport";
import Readline from "@serialport/parser-readline";

const port = new SerialPort('COM9', { baudRate: 57600 });
const parser = port.pipe(new Readline({ delimiter: '\n' }));

const wss = new WebSocketServer({ port: 8081 });

// Read the port data
port.on("open", () => {
    console.log('Serial port opened');
});

wss.on('open', function connection(ws) {
    console.log('Connection opened');
});

// When the WebSocket connection is done
wss.on('connection', function connection(ws) {
    console.log('Connection started');
    // Connect to the Serial Printer and send data each time we receive one
    parser.on('data', data => {
        console.log('send : ', data);
        ws.send(data);
    });
});

wss.on('connectionRejected', function connection(ws) {
    console.log('Connection Opened');
});