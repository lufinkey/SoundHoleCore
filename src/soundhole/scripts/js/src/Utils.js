
module.exports = {
	json_encode: (data) => {
		return JSON.stringify(data);
	},
	json_decode: (data) => {
		return JSON.parse(data);
	}
};
