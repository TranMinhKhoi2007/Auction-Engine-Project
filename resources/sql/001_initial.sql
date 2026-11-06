-- Complete SQLite statements; trigger bodies remain intact between separators.
CREATE TABLE schema_migrations(version INTEGER PRIMARY KEY, applied_at INTEGER NOT NULL);
--@statement
CREATE TABLE users(id INTEGER PRIMARY KEY CHECK(id>0), name TEXT NOT NULL);
--@statement
CREATE TABLE identities(user_id INTEGER PRIMARY KEY REFERENCES users(id), provider TEXT NOT NULL CHECK(provider='email'), subject TEXT NOT NULL UNIQUE, password_hash TEXT NOT NULL);
--@statement
CREATE TABLE wallets(user_id INTEGER PRIMARY KEY REFERENCES users(id), available INTEGER NOT NULL CHECK(typeof(available)='integer' AND available>=0), held INTEGER NOT NULL CHECK(typeof(held)='integer' AND held>=0 AND available<=9223372036854775807-held));
--@statement
CREATE TABLE products(id INTEGER PRIMARY KEY CHECK(id>0), seller_id INTEGER NOT NULL REFERENCES users(id), type TEXT NOT NULL, name TEXT NOT NULL, description TEXT NOT NULL, attributes TEXT NOT NULL);
--@statement
CREATE TABLE auctions(id INTEGER PRIMARY KEY CHECK(id>0), product_id INTEGER NOT NULL UNIQUE REFERENCES products(id), seller_id INTEGER NOT NULL REFERENCES users(id), state TEXT NOT NULL CHECK(state IN ('pending','scheduled','active','closed','cancelled')), starting_price INTEGER NOT NULL CHECK(typeof(starting_price)='integer' AND starting_price>=0), min_increment INTEGER NOT NULL CHECK(typeof(min_increment)='integer' AND min_increment>0), deposit INTEGER NOT NULL CHECK(typeof(deposit)='integer' AND deposit>=0), start_delay_ms INTEGER NOT NULL CHECK(start_delay_ms>=0), duration_ms INTEGER NOT NULL CHECK(duration_ms>0), start_at INTEGER, end_at INTEGER, outcome INTEGER NOT NULL CHECK(outcome BETWEEN 0 AND 4), closed_early INTEGER NOT NULL CHECK(closed_early IN (0,1)), CHECK((start_at IS NULL AND end_at IS NULL AND state IN ('pending','cancelled')) OR (start_at IS NOT NULL AND end_at>start_at)));
--@statement
CREATE TABLE holds(user_id INTEGER NOT NULL REFERENCES users(id), auction_id INTEGER NOT NULL REFERENCES auctions(id), amount INTEGER NOT NULL CHECK(typeof(amount)='integer' AND amount>=0), PRIMARY KEY(user_id,auction_id));
--@statement
CREATE TABLE auction_participants(auction_id INTEGER NOT NULL REFERENCES auctions(id), user_id INTEGER NOT NULL REFERENCES users(id), position INTEGER NOT NULL, PRIMARY KEY(auction_id,user_id), UNIQUE(auction_id,position));
--@statement
CREATE TABLE bids(id INTEGER PRIMARY KEY CHECK(id>0), auction_id INTEGER NOT NULL REFERENCES auctions(id), bidder_id INTEGER NOT NULL REFERENCES users(id), amount INTEGER NOT NULL CHECK(typeof(amount)='integer' AND amount>=0), placed_at INTEGER NOT NULL, position INTEGER NOT NULL, UNIQUE(auction_id,position), FOREIGN KEY(auction_id,bidder_id) REFERENCES auction_participants(auction_id,user_id));
--@statement
CREATE TABLE follows(user_id INTEGER NOT NULL REFERENCES users(id), auction_id INTEGER NOT NULL REFERENCES auctions(id), position INTEGER NOT NULL, PRIMARY KEY(user_id,auction_id), UNIQUE(user_id,position));
--@statement
CREATE TABLE notifications(id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL REFERENCES users(id), auction_id INTEGER NOT NULL REFERENCES auctions(id), payload TEXT NOT NULL);
--@statement
CREATE INDEX notifications_user_id ON notifications(user_id,id);
--@statement
CREATE INDEX auctions_state_end ON auctions(state,end_at);
--@statement
CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, operation TEXT NOT NULL, actor_id INTEGER REFERENCES users(id), auction_id INTEGER REFERENCES auctions(id), committed_at INTEGER NOT NULL);
--@statement
CREATE TABLE ledger(id INTEGER PRIMARY KEY AUTOINCREMENT, transaction_id INTEGER NOT NULL REFERENCES transactions(id), user_id INTEGER NOT NULL REFERENCES users(id), auction_id INTEGER REFERENCES auctions(id), kind TEXT NOT NULL CHECK(kind IN ('TOP_UP','HOLD','REFUND','SETTLE','PROCEEDS','FORFEIT')), available_delta INTEGER NOT NULL, held_delta INTEGER NOT NULL, available_before INTEGER NOT NULL CHECK(available_before>=0), held_before INTEGER NOT NULL CHECK(held_before>=0), available_after INTEGER NOT NULL CHECK(available_after>=0), held_after INTEGER NOT NULL CHECK(held_after>=0), CHECK(available_after-available_before=available_delta), CHECK(held_after-held_before=held_delta), UNIQUE(transaction_id,user_id));
--@statement
CREATE TRIGGER ledger_no_update BEFORE UPDATE OF transaction_id, user_id, kind, available_delta, held_delta, available_before, held_before, available_after, held_after ON ledger BEGIN SELECT RAISE(ABORT,'ledger is append only'); END;
--@statement
CREATE TRIGGER ledger_no_delete BEFORE DELETE ON ledger BEGIN SELECT RAISE(ABORT,'ledger is append only'); END;
--@statement
CREATE INDEX ledger_user_id ON ledger(user_id,id);
--@statement
CREATE TRIGGER bids_no_update BEFORE UPDATE ON bids BEGIN SELECT RAISE(ABORT,'bids are append only'); END;
