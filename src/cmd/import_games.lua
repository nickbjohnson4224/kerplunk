local kp = require('kerplunk')
local sql = require('lsqlite3')
local ffi = require('ffi')
local bit = require('bit')

module = {}

function module.main(sgf_path, db_path)

    -- open SGF game list
    local file = nil
    if sgf_path == nil or sgf_path == '-' then
        file = io.stdin
    else
        file = io.open(sgf_path)
    end

    -- open database
    local db = sql.open(db_path)
    if not db then
        return -1
    end

    db:exec "BEGIN TRANSACTION"

    -- create game record tables
    
    db:exec [[
    CREATE TABLE IF NOT EXISTS games (
        id INTEGER PRIMARY KEY
    )]]
    
    db:exec [[
    CREATE TABLE IF NOT EXISTS game_metadata (
        game_id INTEGER PRIMARY KEY REFERENCES games(id) ON DELETE CASCADE,

        name TEXT,
        date DATE,
        black_name TEXT,
        black_rank TEXT,
        white_name TEXT,
        white_rank TEXT,
        copyright TEXT,
        result TEXT
    )]]

    db:exec [[
    CREATE TABLE IF NOT EXISTS game_setup (
        game_id INTEGER PRIMARY KEY REFERENCES games(id) ON DELETE CASCADE,

        board_size INTEGER DEFAULT 19 NOT NULL,
        ruleset TEXT NOT NULL,
        handicap INTEGER NOT NULL,
        handicaps_packed_le16 BLOB
    )]]

    db:exec [[
    CREATE TABLE IF NOT EXISTS game_movedata (
        game_id INTEGER PRIMARY KEY REFERENCES games(id) ON DELETE CASCADE,

        num_moves INTEGER NOT NULL,
        moves_packed_le16 BLOB
    )]]

    db:exec [[
    CREATE TABLE IF NOT EXISTS game_moves (
        id INTEGER PRIMARY KEY,

        game_id INTEGER REFERENCES games(id) ON DELETE CASCADE,

        move_num INTEGER,

        color CHAR(1),
        row TINYINT,
        col TINYINT
    )]]

    db:exec [[
    CREATE UNIQUE INDEX IF NOT EXISTS game_moves_index ON game_moves (
        game_id,
        move_num ASC
    )]]

    local function cstring(s)
        if s == ffi.NULL then
            return nil
        else
            return ffi.string(s)
        end
    end

    local i = 0
    while true do
        local record = kp.sgf_load(file)
        if record == nil then
            break
        end

        print('inserting game', i)

        -- create new game
        db:exec "INSERT INTO games DEFAULT VALUES"
        local game_id = db:last_insert_rowid()

        -- fill in metadata
        do
            local stmt = db:prepare [[
            INSERT INTO game_metadata (
                game_id,

                name, 
                date,
                black_name, 
                black_rank,
                white_name, 
                white_rank,
                copyright,
                result
            )
            VALUES (?, ?, date(?), ?, ?, ?, ?, ?, ?)
            ]]

            stmt:bind(1, game_id)
            stmt:bind(2, cstring(record.name))
            stmt:bind(3, cstring(record.date))
            stmt:bind(4, cstring(record.black_name))
            stmt:bind(5, cstring(record.black_rank))
            stmt:bind(6, cstring(record.white_name))
            stmt:bind(7, cstring(record.white_rank))
            stmt:bind(8, cstring(record.copyright))
            stmt:bind(9, cstring(record.result))

            stmt:step()
            stmt:finalize()
        end

        -- fill in setup data
        local num_handicaps = tonumber(record.handicap)
        do
            local stmt = db:prepare [[
            INSERT INTO game_setup (
                game_id,

                board_size,
                ruleset,
                handicap,
                handicaps_packed_le16
            ) 
            VALUES (?, 19, ?, ?, ?)
            ]]

            stmt:bind(1, game_id)
            stmt:bind(2, cstring(record.ruleset))
            stmt:bind(3, num_handicaps)
            if num_handicaps > 0 then
                stmt:bind(4, record.handicaps, num_handicaps * 2)
            else
                stmt:bind(4, nil)
            end

            stmt:step()
            stmt:finalize()
        end

        -- fill in move data
        local num_moves = tonumber(record.num_moves)
        do
            local stmt = db:prepare [[
            INSERT INTO game_movedata (
                game_id,
                
                num_moves,
                moves_packed_le16
            )
            VALUES (?, ?, ?)
            ]]

            stmt:bind(1, game_id)
            stmt:bind(2, num_moves)
            if num_moves > 0 then
                stmt:bind(3, record.moves, num_moves * 2)
            else
                stmt:bind(3, nil)
            end

            stmt:step()
            stmt:finalize()
        end

        -- create individual move rows
        do
            local stmt = db:prepare [[
            INSERT INTO game_moves (game_id, move_num, color, row, col)
            VALUES (?, ?, ?, ?, ?)
            ]]

            local color
            if num_handicaps == 0 then
                color = 'B'
            else
                color = 'W'
            end

            for move_num = 0, num_moves-1 do
                local move = record.moves[move_num]

                stmt:clear_bindings()
                stmt:reset()
                stmt:bind(1, game_id)
                stmt:bind(2, move_num)
                stmt:bind(3, color)
                
                if move == 0 then
                    stmt:bind(4, nil)
                    stmt:bind(5, nil)
                else
                    stmt:bind(4, bit.rshift(move, 8))
                    stmt:bind(5, bit.band(move, 255))
                end

                stmt:step()

                if color == 'B' then
                    color = 'W'
                else
                    color = 'B'
                end
            end

            stmt:finalize()
        end

        i = i + 1
    end

    db:exec "END TRANSACTION"
end

return module
