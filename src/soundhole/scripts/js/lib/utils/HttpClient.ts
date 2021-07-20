
import { URL } from 'url';
import http from 'http';
import https from 'https';

type HttpClient = {
	request: (url: string | URL, opts: http.RequestOptions, callback: (res: http.IncomingMessage) => void) => http.ClientRequest
}

export type HttpRequest = {
	url: string
	method: 'GET' | 'POST' | 'PUT' | 'DELETE' | 'OPTIONS'
	headers?: http.OutgoingHttpHeaders
	data?: Buffer | null
}

export type HttpResponse = {
	statusCode: number
	statusMessage: string
	headers: http.IncomingHttpHeaders,
	data: Uint8Array
}

export function performHttpRequest(request: HttpRequest, callback: (error: Error | null, response: HttpResponse | null) => void) {
	const url = new URL(request.url);
	let client: HttpClient;
	if(url.protocol === 'http:') {
		client = http;
	} else if(url.protocol === 'https:') {
		client = https;
	} else {
		throw new Error("Invalid protocol "+url.protocol);
	}
	const buffers: Buffer[] = [];
	let errored = false;
	const req = client.request(url, {
		method: request.method,
		headers: request.headers
	}, (res) => {
		res.on('data', (chunk) => {
			buffers.push(chunk);
		});
		res.on('end', () => {
			if(errored) {
				return;
			}
			const buffer = Buffer.concat(buffers);
			const arrayBuffer = new ArrayBuffer(buffer.length);
			const data = new Uint8Array(arrayBuffer);
			for(let i=0; i<buffer.length; i++) {
				data[i] = buffer[i];
			}
			callback(null, {
				statusCode: res.statusCode as number,
				statusMessage: res.statusMessage as string,
				headers: res.headers,
				data: data
			});
		});
	});

	req.once('error', (error) => {
		errored = true;
		callback(error, null);
	});
	
	if(request.data != null) {
		req.write(request.data);
	}
	req.end();
}
