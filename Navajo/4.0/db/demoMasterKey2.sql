
INSERT INTO KeySets VALUES(
  'd796d7bd-ae33-415a-a454-df15a58ebbcd',
  'Demo KeySet'
);

INSERT INTO KeyGroups VALUES(
  '807cb358-961e-4dd7-b707-72a271271af5',
  'Demo KeyGroup'
);

INSERT INTO Keys VALUES(
  'd0ad1710-d120-400f-b7b4-c6e4d4b132ba',
  'd796d7bd-ae33-415a-a454-df15a58ebbcd',
  '807cb358-961e-4dd7-b707-72a271271af5',
  'Alice',
  'dec_d0ad1710d120400fb7b4c6e4d4b132ba',
  0,
  0,
  0,
  1,
  268435456,
  0
);

INSERT INTO Keys VALUES(
  '42326180-adfd-4c9e-9b79-48c9489dd6f4',
  'd796d7bd-ae33-415a-a454-df15a58ebbcd',
  '807cb358-961e-4dd7-b707-72a271271af5',
  'Bob',
  'enc_42326180adfd4c9e9b7948c9489dd6f4',
  1,
  0,
  0,
  1,
  268435456,
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
  'd0ad1710-d120-400f-b7b4-c6e4d4b132ba',
  1
);

INSERT INTO KeyChunks VALUES(
  2,
  '42326180-adfd-4c9e-9b79-48c9489dd6f4',
  2
);

INSERT INTO KeyOffsets VALUES(
  'd0ad1710-d120-400f-b7b4-c6e4d4b132ba',
  1,
  0,
  0
);

INSERT INTO KeyOffsets VALUES(
  '42326180-adfd-4c9e-9b79-48c9489dd6f4',
  2,
  0,
  0
);
