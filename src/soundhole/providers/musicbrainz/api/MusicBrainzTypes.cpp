//
//  MusicBrainzTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/1/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "MusicBrainzTypes.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	MusicBrainzPeriod MusicBrainzPeriod::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzPeriod::fromNapiObject requires an object");
		}
		return MusicBrainzPeriod{
			.begin = jsutils::stringFromNapiValue(obj.Get("begin")),
			.end = jsutils::stringFromNapiValue(obj.Get("end")),
			.ended = jsutils::optBoolFromNapiValue(obj.Get("ended"))
		};
	}

	MusicBrainzPeriod MusicBrainzPeriod::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzPeriod::fromJson requires an object");
		}
		return MusicBrainzPeriod{
			.begin = jsutils::stringFromJson(json["begin"]),
			.end = jsutils::stringFromJson(json["end"]),
			.ended = jsutils::optBoolFromJson(json["ended"])
		};
	}



	MusicBrainzURL MusicBrainzURL::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzURL::fromNapiObject requires an object");
		}
		return MusicBrainzURL{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.resource = jsutils::nonNullStringPropFromNapiObject(obj, "resource")
		};
	}

	MusicBrainzURL MusicBrainzURL::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzURL::fromJson requires an object");
		}
		return MusicBrainzURL{
			.id = jsutils::nonNullStringPropFromJson(json, "id"),
			.resource = jsutils::nonNullStringPropFromJson(json, "resource")
		};
	}



	MusicBrainzArea MusicBrainzArea::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzArea::fromNapiObject requires an object");
		}
		return MusicBrainzArea{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.typeId = jsutils::stringFromNapiValue(obj.Get("type-id")),
			.type = jsutils::stringFromNapiValue(obj.Get("type")),
			.name = jsutils::nonNullStringPropFromNapiObject(obj, "name"),
			.sortName = jsutils::stringFromNapiValue(obj.Get("sort-name")),
			.disambiguation = jsutils::stringFromNapiValue(obj.Get("disambiguation")),
			.iso_3166_1_codes = jsutils::optArrayListFromNapiValue(obj.Get("iso-3166-1-codes"), [](Napi::Value code) {
				return jsutils::stringFromNapiValue(code);
			})
		};
	}

	MusicBrainzArea MusicBrainzArea::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzArea::fromJson requires an object");
		}
		return MusicBrainzArea{
			.id = jsutils::nonNullStringPropFromJson(json, "id"),
			.typeId = jsutils::stringFromJson(json["type-id"]),
			.type = jsutils::stringFromJson(json["type"]),
			.name = jsutils::nonNullStringPropFromJson(json, "name"),
			.sortName = jsutils::stringFromJson(json["sort-name"]),
			.disambiguation = jsutils::stringFromJson(json["disambiguation"]),
			.iso_3166_1_codes = jsutils::optArrayListFromJson(json["iso-3166-1-codes"], [](const Json& code) {
				return jsutils::stringFromJson(code);
			})
		};
	}



	MusicBrainzAlias MusicBrainzAlias::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzAlias::fromNapiObject requires an object");
		}
		return MusicBrainzAlias{
			.name = jsutils::nonNullStringPropFromNapiObject(obj, "name"),
			.sortName = jsutils::nonNullStringPropFromNapiObject(obj, "sort-name"),
			.type = jsutils::stringFromNapiValue(obj.Get("type")),
			.locale = jsutils::stringFromNapiValue(obj.Get("locale")),
			.primary = jsutils::optBoolFromNapiValue(obj.Get("primary")),
			.ended = jsutils::optBoolFromNapiValue(obj.Get("ended"))
		};
	}

	MusicBrainzAlias MusicBrainzAlias::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzAlias::fromJson requires an object");
		}
		return MusicBrainzAlias{
			.name = jsutils::nonNullStringPropFromJson(json, "name"),
			.sortName = jsutils::nonNullStringPropFromJson(json, "sort-name"),
			.type = jsutils::stringFromJson(json["type"]),
			.locale = jsutils::stringFromJson(json["locale"]),
			.primary = jsutils::optBoolFromJson(json["primary"]),
			.ended = jsutils::optBoolFromJson(json["ended"])
		};
	}



	MusicBrainzRelation MusicBrainzRelation::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzRelation::fromNapiObject requires an object");
		}
		auto targetType = jsutils::stringFromNapiValue(obj.Get("target-type"));
		if(targetType.empty()) {
			throw std::invalid_argument("MusicBrainzRelation.target-type cannot be empty");
		}
		auto itemObj = obj.Get(targetType.c_str()).As<Napi::Object>();
		if(itemObj.IsEmpty() || itemObj.IsNull() || itemObj.IsUndefined() || !itemObj.IsObject()) {
			throw std::invalid_argument("missing property '"+targetType+"' for MusicBrainzRelation");
		}
		auto item = ItemVariant((std::nullptr_t)nullptr);
		if(targetType == "url") {
			item = std::make_shared<MusicBrainzURL>(MusicBrainzURL::fromNapiObject(itemObj));
		} else if(targetType == "release") {
			item = std::make_shared<MusicBrainzRelease>(MusicBrainzRelease::fromNapiObject(itemObj));
		} else if(targetType == "artist") {
			item = std::make_shared<MusicBrainzArtist>(MusicBrainzArtist::fromNapiObject(itemObj));
		}
		return MusicBrainzRelation{
			.targetType = targetType,
			.typeId = jsutils::stringFromNapiValue(obj.Get("type-id")),
			.type = jsutils::nonNullStringPropFromNapiObject(obj, "type"),
			.direction = jsutils::nonNullStringPropFromNapiObject(obj, "direction"),
			.targetCredit = jsutils::stringFromNapiValue(obj.Get("target-credit")),
			.sourceCredit = jsutils::stringFromNapiValue(obj.Get("source-credit")),
			.begin = jsutils::stringFromNapiValue(obj.Get("begin")),
			.end = jsutils::stringFromNapiValue(obj.Get("end")),
			.ended = jsutils::optBoolFromNapiValue(obj.Get("ended")),
			.item = item
		};
	}

	MusicBrainzRelation MusicBrainzRelation::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzRelation::fromJson requires an object");
		}
		auto targetType = jsutils::stringFromJson(json["target-type"]);
		if(targetType.empty()) {
			throw std::invalid_argument("MusicBrainzRelation.target-type cannot be empty");
		}
		auto itemJson = json[targetType];
		if(itemJson.is_null() || !itemJson.is_object()) {
			throw std::invalid_argument("missing property '"+targetType+"' for MusicBrainzRelation");
		}
		auto item = ItemVariant((std::nullptr_t)nullptr);
		if(targetType == "url") {
			item = std::make_shared<MusicBrainzURL>(MusicBrainzURL::fromJson(itemJson));
		} else if(targetType == "release") {
			item = std::make_shared<MusicBrainzRelease>(MusicBrainzRelease::fromJson(itemJson));
		} else if(targetType == "artist") {
			item = std::make_shared<MusicBrainzArtist>(MusicBrainzArtist::fromJson(itemJson));
		}
		return MusicBrainzRelation{
			.targetType = targetType,
			.typeId = jsutils::stringFromJson(json["type-id"]),
			.type = jsutils::nonNullStringPropFromJson(json, "type"),
			.direction = jsutils::nonNullStringPropFromJson(json, "direction"),
			.targetCredit = jsutils::stringFromJson(json["target-credit"]),
			.sourceCredit = jsutils::stringFromJson(json["source-credit"]),
			.begin = jsutils::stringFromJson(json["begin"]),
			.end = jsutils::stringFromJson(json["end"]),
			.ended = jsutils::optBoolFromJson(json["ended"]),
			.item = item
		};
	}



	MusicBrainzArtist MusicBrainzArtist::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzArtist::fromNapiObject requires an object");
		}
		return MusicBrainzArtist{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.name = jsutils::nonNullStringPropFromNapiObject(obj, "name"),
			.sortName = jsutils::stringFromNapiValue(obj.Get("sort-name")),
			.disambiguation = jsutils::stringFromNapiValue(obj.Get("disambiguation")),
			.country = jsutils::stringFromNapiValue(obj.Get("country")),
			.aliases = jsutils::optArrayListFromNapiValue(obj.Get("aliases"), [](Napi::Value aliasObj) {
				return MusicBrainzAlias::fromNapiObject(aliasObj.As<Napi::Object>());
			}),
			.area = jsutils::optValueFromNapiValue(obj.Get("area"), [](Napi::Value areaObj) {
				return MusicBrainzArea::fromNapiObject(areaObj.As<Napi::Object>());
			}),
			.beginArea = jsutils::optValueFromNapiValue(obj.Get("begin-area"), [](Napi::Value areaObj) {
				return MusicBrainzArea::fromNapiObject(areaObj.As<Napi::Object>());
			}),
			.endArea = jsutils::optValueFromNapiValue(obj.Get("end-area"), [](Napi::Value areaObj) {
				return MusicBrainzArea::fromNapiObject(areaObj.As<Napi::Object>());
			}),
			.lifeSpan = jsutils::optValueFromNapiValue(obj.Get("life-span"), [](Napi::Value lifeSpanObj) {
				return MusicBrainzPeriod::fromNapiObject(lifeSpanObj.As<Napi::Object>());
			}),
			.relations = jsutils::optArrayListFromNapiValue(obj.Get("relations"), [](Napi::Value relationObj) {
				return MusicBrainzRelation::fromNapiObject(relationObj.As<Napi::Object>());
			}),
			.releases = jsutils::optArrayListFromNapiValue(obj.Get("releases"), [](Napi::Value releaseObj) {
				return MusicBrainzRelease::fromNapiObject(releaseObj.As<Napi::Object>());
			}),
			.releaseGroups = jsutils::optArrayListFromNapiValue(obj.Get("release-groups"), [](Napi::Value releaseGroupObj) {
				return MusicBrainzReleaseGroup::fromNapiObject(releaseGroupObj.As<Napi::Object>());
			})
		};
	}

	MusicBrainzArtist MusicBrainzArtist::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzArtist::fromJson requires an object");
		}
		return MusicBrainzArtist{
			.id = jsutils::nonNullStringPropFromJson(json, "id"),
			.name = jsutils::nonNullStringPropFromJson(json, "name"),
			.sortName = jsutils::stringFromJson(json["sort-name"]),
			.disambiguation = jsutils::stringFromJson(json["disambiguation"]),
			.country = jsutils::stringFromJson(json["country"]),
			.aliases = jsutils::optArrayListFromJson(json["aliases"], [](const Json& aliasJson) {
				return MusicBrainzAlias::fromJson(aliasJson);
			}),
			.area = ([&]() -> Optional<MusicBrainzArea> {
				auto areaJson = json["area"];
				if(areaJson.is_null()) {
					return std::nullopt;
				}
				return MusicBrainzArea::fromJson(areaJson);
			})(),
			.beginArea = ([&]() -> Optional<MusicBrainzArea> {
				auto areaJson = json["begin-area"];
				if(areaJson.is_null()) {
					return std::nullopt;
				}
				return MusicBrainzArea::fromJson(areaJson);
			})(),
			.endArea = ([&]() -> Optional<MusicBrainzArea> {
				auto areaJson = json["end-area"];
				if(areaJson.is_null()) {
					return std::nullopt;
				}
				return MusicBrainzArea::fromJson(areaJson);
			})(),
			.lifeSpan = ([&]() -> Optional<MusicBrainzPeriod> {
				auto spanJson = json["life-span"];
				if(spanJson.is_null()) {
					return std::nullopt;
				}
				return MusicBrainzPeriod::fromJson(spanJson);
			})(),
			.relations = jsutils::optArrayListFromJson(json["relations"], [](const Json& relationJson) {
				return MusicBrainzRelation::fromJson(relationJson);
			}),
			.releases = jsutils::optArrayListFromJson(json["releases"], [](const Json& releaseJson) {
				return MusicBrainzRelease::fromJson(releaseJson);
			}),
			.releaseGroups = jsutils::optArrayListFromJson(json["release-groups"], [](const Json& releaseGroupJson) {
				return MusicBrainzReleaseGroup::fromJson(releaseGroupJson);
			})
		};
	}



	MusicBrainzArtistCredit MusicBrainzArtistCredit::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzArtistCredit::fromNapiObject requires an object");
		}
		return MusicBrainzArtistCredit{
			.artist = MusicBrainzArtist::fromNapiObject(obj.Get("artist").As<Napi::Object>()),
			.joinphrase = jsutils::stringFromNapiValue(obj.Get("joinphrase")),
			.name = jsutils::stringFromNapiValue(obj.Get("name"))
		};
	}

	MusicBrainzArtistCredit MusicBrainzArtistCredit::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzArtistCredit::fromJson requires an object");
		}
		return MusicBrainzArtistCredit{
			.artist = MusicBrainzArtist::fromJson(json["artist"]),
			.joinphrase = jsutils::stringFromJson(json["joinphrase"]),
			.name = jsutils::stringFromJson(json["name"])
		};
	}



	MusicBrainzRelease MusicBrainzRelease::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzRelease::fromNapiObject requires an object");
		}
		return MusicBrainzRelease{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.title = jsutils::stringFromNapiValue(obj.Get("title")),
			.disambiguation = jsutils::stringFromNapiValue(obj.Get("disambiguation")),
			.asin = jsutils::stringFromNapiValue(obj.Get("asin")),
			.statusId = jsutils::stringFromNapiValue(obj.Get("status-id")),
			.status = jsutils::stringFromNapiValue(obj.Get("status")),
			.date = jsutils::stringFromNapiValue(obj.Get("date")),
			.country = jsutils::stringFromNapiValue(obj.Get("country")),
			.quality = jsutils::stringFromNapiValue(obj.Get("quality")),
			.barcode = jsutils::stringFromNapiValue(obj.Get("barcode")),
			.media = jsutils::arrayListFromNapiValue(obj.Get("media"), [](Napi::Value mediaObj) {
				return MusicBrainzMedia::fromNapiObject(mediaObj.As<Napi::Object>());
			}),
			.coverArtArchive = jsutils::optValueFromNapiValue(obj.Get("cover-art-archive"), [](Napi::Value archiveObj) {
				return MusicBrainzCoverArtArchive::fromNapiObject(archiveObj.As<Napi::Object>());
			}),
			.artistCredit = jsutils::optArrayListFromNapiValue(obj.Get("artist-credit"), [](Napi::Value creditObj) {
				return MusicBrainzArtistCredit::fromNapiObject(creditObj.As<Napi::Object>());
			}),
			.releaseEvents = jsutils::optArrayListFromNapiValue(obj.Get("release-events"), [](Napi::Value eventObj) {
				return MusicBrainzReleaseEvent::fromNapiObject(eventObj.As<Napi::Object>());
			}),
			.releaseGroup = ([&]() -> std::shared_ptr<MusicBrainzReleaseGroup> {
				auto groupObj = obj.Get("release-group");
				if(groupObj.IsEmpty() || groupObj.IsNull() || groupObj.IsUndefined() || !groupObj.IsObject()) {
					return nullptr;
				}
				return std::make_shared<MusicBrainzReleaseGroup>(MusicBrainzReleaseGroup::fromNapiObject(groupObj.As<Napi::Object>()));
			})()
		};
	}

	MusicBrainzRelease MusicBrainzRelease::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzRelease::fromJson requires an object");
		}
		return MusicBrainzRelease{
			.id = jsutils::nonNullStringPropFromJson(json, "id"),
			.title = jsutils::stringFromJson(json["title"]),
			.disambiguation = jsutils::stringFromJson(json["disambiguation"]),
			.asin = jsutils::stringFromJson(json["asin"]),
			.statusId = jsutils::stringFromJson(json["status-id"]),
			.status = jsutils::stringFromJson(json["status"]),
			.date = jsutils::stringFromJson(json["date"]),
			.country = jsutils::stringFromJson(json["country"]),
			.quality = jsutils::stringFromJson(json["quality"]),
			.barcode = jsutils::stringFromJson(json["barcode"]),
			.media = jsutils::arrayListFromJson(json["media"], [](const Json& mediaJson) {
				return MusicBrainzMedia::fromJson(mediaJson);
			}),
			.coverArtArchive = ([&]() -> Optional<MusicBrainzCoverArtArchive> {
				auto archiveJson = json["cover-art-archive"];
				if(archiveJson.is_null()) {
					return std::nullopt;
				}
				return MusicBrainzCoverArtArchive::fromJson(archiveJson);
			})(),
			.artistCredit = jsutils::optArrayListFromJson(json["artist-credit"], [](const Json& creditJson) {
				return MusicBrainzArtistCredit::fromJson(creditJson);
			}),
			.releaseEvents = jsutils::optArrayListFromJson(json["release-events"], [](const Json& eventJson) {
				return MusicBrainzReleaseEvent::fromJson(eventJson);
			}),
			.releaseGroup = ([&]() -> std::shared_ptr<MusicBrainzReleaseGroup> {
				auto groupJson = json["release-group"];
				if(groupJson.is_null() || !groupJson.is_object()) {
					return nullptr;
				}
				return std::make_shared<MusicBrainzReleaseGroup>(MusicBrainzReleaseGroup::fromJson(groupJson));
			})()
		};
	}



	MusicBrainzReleaseEvent MusicBrainzReleaseEvent::fromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzReleaseEvent::fromNapiObject requires an object");
		}
		return MusicBrainzReleaseEvent{
			.area = jsutils::optValueFromNapiValue(obj.Get("area"), [](Napi::Value areaObj) {
				return MusicBrainzArea::fromNapiObject(areaObj.As<Napi::Object>());
			}),
			.date = jsutils::optStringFromNapiValue(obj.Get("date"))
		};
	}

	MusicBrainzReleaseEvent MusicBrainzReleaseEvent::fromJson(const Json& json) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzReleaseEvent::fromJson requires an object");
		}
		return MusicBrainzReleaseEvent{
			.area = ([&]() -> Optional<MusicBrainzArea> {
				auto areaJson = json["area"];
				if(areaJson.is_null()) {
					return std::nullopt;
				}
				return MusicBrainzArea::fromJson(areaJson);
			})(),
			.date = jsutils::optStringFromJson(json["date"])
		};
	}



	MusicBrainzRecording MusicBrainzRecording::fromNapiObject(Napi::Object obj) {
		return MusicBrainzRecording{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.title = jsutils::stringFromNapiValue(obj.Get("title")),
			.disambiguation = jsutils::stringFromNapiValue(obj.Get("disambiguation")),
			.length = jsutils::optDoubleFromNapiValue(obj.Get("length")),
			.video = jsutils::boolFromNapiValue(obj.Get("video")),
			.releases = jsutils::optArrayListFromNapiValue(obj.Get("releases"), [](Napi::Value releaseObj) {
				return MusicBrainzRelease::fromNapiObject(releaseObj.As<Napi::Object>());
			}),
			.relations = jsutils::optArrayListFromNapiValue(obj.Get("relations"), [](Napi::Value relationObj) {
				return MusicBrainzRelation::fromNapiObject(relationObj.As<Napi::Object>());
			}),
			.artistCredit = jsutils::optArrayListFromNapiValue(obj.Get("artist-credit"), [](Napi::Value creditObj) {
				return MusicBrainzArtistCredit::fromNapiObject(creditObj.As<Napi::Object>());
			}),
			.aliases = jsutils::optArrayListFromNapiValue(obj.Get("aliases"), [](Napi::Value aliasObj) {
				return MusicBrainzAlias::fromNapiObject(aliasObj.As<Napi::Object>());
			})
		};
	}

	MusicBrainzRecording MusicBrainzRecording::fromJson(const Json& json) {
		return MusicBrainzRecording{
			.id = jsutils::nonNullStringPropFromJson(json, "id"),
			.title = jsutils::stringFromJson(json["title"]),
			.disambiguation = jsutils::stringFromJson(json["disambiguation"]),
			.length = jsutils::optDoubleFromJson(json["length"]),
			.video = jsutils::optBoolFromJson(json["video"])
				.valueOrThrow(std::invalid_argument("missing property 'video' for MusicBrainzRecording")),
			.releases = jsutils::optArrayListFromJson(json["releases"], [](const Json& releaseJson) {
				return MusicBrainzRelease::fromJson(releaseJson);
			}),
			.relations = jsutils::optArrayListFromJson(json["relations"], [](const Json& relationJson) {
				return MusicBrainzRelation::fromJson(relationJson);
			}),
			.artistCredit = jsutils::optArrayListFromJson(json["artist-credit"], [](const Json& creditJson) {
				return MusicBrainzArtistCredit::fromJson(creditJson);
			}),
			.aliases = jsutils::optArrayListFromJson(json["aliases"], [](const Json& aliasJson) {
				return MusicBrainzAlias::fromJson(aliasJson);
			})
		};
	}



	MusicBrainzTrack MusicBrainzTrack::fromNapiObject(Napi::Object obj) {
		return MusicBrainzTrack{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.number = jsutils::nonNullSizePropFromNapiObject(obj, "number"),
			.title = jsutils::stringFromNapiValue(obj.Get("title")),
			.length = jsutils::optDoubleFromNapiValue(obj.Get("length")),
			.artistCredit = jsutils::optArrayListFromNapiValue(obj.Get("artist-credit"), [](Napi::Value creditObj) {
				return MusicBrainzArtistCredit::fromNapiObject(creditObj.As<Napi::Object>());
			})
		};
	}

	MusicBrainzTrack MusicBrainzTrack::fromJson(const Json& json) {
		return MusicBrainzTrack{
			.id = jsutils::nonNullStringPropFromJson(json, "id"),
			.number = jsutils::nonNullSizePropFromJson(json, "number"),
			.title = jsutils::stringFromJson(json["title"]),
			.length = jsutils::optDoubleFromJson(json["length"]),
			.artistCredit = jsutils::optArrayListFromJson(json["artist-credit"], [](const Json& creditJson) {
				return MusicBrainzArtistCredit::fromJson(creditJson);
			})
		};
	}



	MusicBrainzMedia MusicBrainzMedia::fromNapiObject(Napi::Object obj) {
		return MusicBrainzMedia{
			.title = jsutils::stringFromNapiValue(obj.Get("title")),
			.position = jsutils::nonNullSizePropFromNapiObject(obj, "position"),
			.tracks = jsutils::arrayListFromNapiValue(obj, [](Napi::Value trackObj) {
				return MusicBrainzTrack::fromNapiObject(trackObj.As<Napi::Object>());
			}),
			.trackCount = jsutils::nonNullSizePropFromNapiObject(obj, "track-count"),
			.trackOffset = jsutils::nonNullSizePropFromNapiObject(obj, "track-offset")
		};
	}

	MusicBrainzMedia MusicBrainzMedia::fromJson(const Json& json) {
		return MusicBrainzMedia{
			.title = jsutils::stringFromJson(json["title"]),
			.position = jsutils::nonNullSizePropFromJson(json, "position"),
			.tracks = jsutils::arrayListFromJson(json, [](const Json& trackJson) {
				return MusicBrainzTrack::fromJson(trackJson);
			}),
			.trackCount = jsutils::nonNullSizePropFromJson(json, "track-count"),
			.trackOffset = jsutils::nonNullSizePropFromJson(json, "track-offset")
		};
	}
}
