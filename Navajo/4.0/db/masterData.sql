
INSERT INTO KeySets VALUES(
  'af6dd043-db3e-464c-8c5c-23210a8b5a3f',
  'Default KeySet'
);

INSERT INTO KeyGroups VALUES(
  '691642ba-a50d-41f2-b73b-57582dcf1ca8',
  'Default KeyGroup'
);

INSERT INTO Keys VALUES(
  'e5c3fa33-2c5f-427d-bd1e-1703d36a1cef',
  'af6dd043-db3e-464c-8c5c-23210a8b5a3f',
  '691642ba-a50d-41f2-b73b-57582dcf1ca8',
  'Test Key #1',
  'enc_e5c3fa332c5f427dbd1e1703d36a1cef',
  1,
  0,
  0,
  1,
  100000000, /* 100MB */
  0
);

INSERT INTO Keys VALUES(
  '83919e62-bd92-4c48-9be9-bfac6f53bcc3',
  'af6dd043-db3e-464c-8c5c-23210a8b5a3f',
  '691642ba-a50d-41f2-b73b-57582dcf1ca8',
  'Test Key #2',
  'dec_83919e62bd924c489be9bfac6f53bcc3',
  0,
  0,
  0,
  1,
  100000000, /* 100MB */
  0
);

INSERT INTO Keys VALUES(
  'de6feaae-422d-479a-9a42-f8544dc16805',
  'af6dd043-db3e-464c-8c5c-23210a8b5a3f',
  '691642ba-a50d-41f2-b73b-57582dcf1ca8',
  'Test Key #3',
  'dec_de6feaae422d479a9a42f8544dc16805',
  0,
  0,
  0,
  1,
  100000000, /* 100MB */
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

INSERT INTO Chunks VALUES(
  3,
  ''
);

INSERT INTO KeyChunks VALUES(
  1,
  'e5c3fa33-2c5f-427d-bd1e-1703d36a1cef',
  1
);

INSERT INTO KeyChunks VALUES(
  2,
  '83919e62-bd92-4c48-9be9-bfac6f53bcc3',
  2
);

INSERT INTO KeyChunks VALUES(
  3,
  'de6feaae-422d-479a-9a42-f8544dc16805',
  3
);

INSERT INTO KeyOffsets VALUES(
  'e5c3fa33-2c5f-427d-bd1e-1703d36a1cef',
  1,
  0,
  0
);

INSERT INTO KeyOffsets VALUES(
  '83919e62-bd92-4c48-9be9-bfac6f53bcc3',
  2,
  0,
  0
);

INSERT INTO KeyOffsets VALUES(
  'de6feaae-422d-479a-9a42-f8544dc16805',
  3,
  0,
  0
);

INSERT INTO KeyProperties VALUES(
  1,
  'e5c3fa33-2c5f-427d-bd1e-1703d36a1cef',
  'string',
  'ExternalId',
  '123-45-6789'
);

INSERT INTO KeyProperties VALUES(
  2,
  '83919e62-bd92-4c48-9be9-bfac6f53bcc3',
  'string',
  'ExternalId',
  '456-78-9101'
);
