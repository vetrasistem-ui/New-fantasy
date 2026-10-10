-- Evidence only. assetA/assetB encode serverId:clientId. E/S edges are directed.
SELECT serverId, clientId, houseTileUses FROM assets
WHERE houseTileUses>0 ORDER BY houseTileUses DESC LIMIT 30;

SELECT serverId, clientId, totalUses, houseTileUses,
       1.0*houseTileUses/totalUses AS houseRatio
FROM assets WHERE totalUses>=50 ORDER BY houseRatio DESC, totalUses DESC LIMIT 30;

SELECT serverId,clientId,totalUses,houseTileUses,uniqueHouses FROM assets
WHERE serverId IN (22280,23131,34675,34934,33889,10177);

-- Bind :asset to each serverId:clientId returned above; combine incoming/outgoing.
SELECT neighbor,SUM(count) AS uses FROM (
 SELECT assetB AS neighbor,count FROM adjacency WHERE assetA=:asset
 UNION ALL SELECT assetA,count FROM adjacency WHERE assetB=:asset
) GROUP BY neighbor ORDER BY uses DESC,neighbor LIMIT 10;

SELECT assetA,assetB,SUM(sameHouseCount) AS houseEdges FROM adjacency
WHERE sameHouseCount>0 GROUP BY assetA,assetB ORDER BY houseEdges DESC LIMIT 30;

-- A candidate lies between two occurrences of a supplied wall candidate along
-- the same axis, on the same nonzero houseId. IDs are hypotheses, not labels.
-- Bind :wall to a candidate serverId. Nested contents are excluded.
SELECT center.serverId,center.clientId,COUNT(DISTINCT middle.tileId) AS examples
FROM tiles middle JOIN items center ON center.tileId=middle.tileId AND center.depth=0
WHERE middle.houseId!=0 AND center.serverId!=:wall AND (
 (EXISTS(SELECT 1 FROM tiles t JOIN items i ON i.tileId=t.tileId
   WHERE t.x=middle.x-1 AND t.y=middle.y AND t.z=middle.z AND t.houseId=middle.houseId AND i.depth=0 AND i.serverId=:wall)
 AND EXISTS(SELECT 1 FROM tiles t JOIN items i ON i.tileId=t.tileId
   WHERE t.x=middle.x+1 AND t.y=middle.y AND t.z=middle.z AND t.houseId=middle.houseId AND i.depth=0 AND i.serverId=:wall))
 OR
 (EXISTS(SELECT 1 FROM tiles t JOIN items i ON i.tileId=t.tileId
   WHERE t.x=middle.x AND t.y=middle.y-1 AND t.z=middle.z AND t.houseId=middle.houseId AND i.depth=0 AND i.serverId=:wall)
 AND EXISTS(SELECT 1 FROM tiles t JOIN items i ON i.tileId=t.tileId
   WHERE t.x=middle.x AND t.y=middle.y+1 AND t.z=middle.z AND t.houseId=middle.houseId AND i.depth=0 AND i.serverId=:wall))
) GROUP BY center.serverId,center.clientId ORDER BY examples DESC LIMIT 30;
