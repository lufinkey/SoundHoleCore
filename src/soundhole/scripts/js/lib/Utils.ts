
export const json_encode = (data: any): string => {
	return JSON.stringify(data);
}

export const json_decode = (data: string): any => {
	return JSON.parse(data);
}
