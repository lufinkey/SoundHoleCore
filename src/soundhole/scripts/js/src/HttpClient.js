
const { URL } = require('url');
const http = require('http');
const https = require('https');

function performHttpRequest(request, callback) {
	const url = new URL(request.url);
	let client = null;
	if(url.protocol === 'http:') {
		client = http;
	} else if(url.protocol === 'https:') {
		client = https;
	} else {
		throw new Error("Invalid protocol "+url.protocol);
	}
	const buffers = [];
	const req = client.request(url, {
		method: request.method,
		headers: request.headers
	}, (res) => {
		res.on('data', (chunk) => {
			buffers.push(chunk);
		});
		res.on('end', () => {
			const data = Buffer.concat(buffers);
			buffers = null;
			callback(null, {
				statusCode: res.statusCode,
				statusMessage: res.statusMessage,
				headers: res.headers,
				data: data
			});
		});
	});

	req.once('error', (error) => {
		callback(error, null);
	});
	
	if(request.data != null) {
		req.write(request.data);
	}
	req.end();
}


module.exports = {
	performHttpRequest
}
