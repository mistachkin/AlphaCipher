
INSERT INTO KeySets VALUES(
  '$keySetId',
  '$keySetName'
);

INSERT INTO KeyGroups VALUES(
  '$keyGroupId',
  '$keyGroupName'
);

INSERT INTO Keys VALUES(
  '$keyId(1)',
  '$keySetId',
  '$keyGroupId',
  '$keyName(1)',
  '[expr {$keyMediaId eq $keyId(1) ? "enc" : "dec"}]_[string map [list - {}] $keyId(1)]',
  [expr {$keyMediaId eq $keyId(1) ? 1 : 0}],
  0,
  0,
  1,
  $keySize,
  0
);

INSERT INTO Keys VALUES(
  '$keyId(2)',
  '$keySetId',
  '$keyGroupId',
  '$keyName(2)',
  '[expr {$keyMediaId eq $keyId(2) ? "enc" : "dec"}]_[string map [list - {}] $keyId(2)]',
  [expr {$keyMediaId eq $keyId(2) ? 1 : 0}],
  0,
  0,
  1,
  $keySize,
  0
);

INSERT INTO Chunks VALUES(
  1,
  ''
);

INSERT INTO Chunks VALUES(
  2,
  ''
);

INSERT INTO KeyChunks VALUES(
  1,
  '$keyId(1)',
  1
);

INSERT INTO KeyChunks VALUES(
  2,
  '$keyId(2)',
  2
);

INSERT INTO KeyOffsets VALUES(
  '$keyId(1)',
  1,
  0,
  0
);

INSERT INTO KeyOffsets VALUES(
  '$keyId(2)',
  2,
  0,
  0
);
