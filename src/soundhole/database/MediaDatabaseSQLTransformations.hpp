//
//  MediaDatabaseSQLTransformations.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::sql {

Json transformDBTrack(Json json);
Json transformDBTrackCollection(Json collectionJson, Json ownerJson);
Json transformDBTrackCollectionItem(Json collectionItemJson, Json trackJson);
Json combineDBTrackCollectionAndItems(Json collectionJson, LinkedList<Json> itemsJson);
Json transformDBArtist(Json json);
Json transformDBUserAccount(Json json);
Json transformDBSavedTrack(Json savedTrackJson, Json trackJson);
Json transformDBSavedAlbum(Json savedAlbumJson, Json albumJson);
Json transformDBSavedPlaylist(Json savedPlaylistJson, Json playlistJson, Json ownerJson);

}
