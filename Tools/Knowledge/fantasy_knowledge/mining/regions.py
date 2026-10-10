class RegionMiner:
    """Future 8-neighbor connected-region analysis; intentionally no semantics."""
    neighbors = ((-1,-1),(0,-1),(1,-1),(-1,0),(1,0),(-1,1),(0,1),(1,1))

    def mine(self, evidence_connection, family_resolver):
        raise NotImplementedError('Region mining is outside V1 statistical gate')
