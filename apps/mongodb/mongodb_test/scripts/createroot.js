print('Create Root Account ...');
db.createUser({user:"root", pwd:"Estuary12#$", roles:[{role:"userAdminAnyDatabase", db:"admin"}]});
db.shutdownServer();
