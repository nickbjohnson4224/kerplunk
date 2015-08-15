local kp = require('kerplunk')
local sql = require('lsqlite3')
local ffi = require('ffi')

module = {}

function module.main(db_path)

    -- open database
    local db = sql.open(db_path)
    if not db then
        return -1
    end

    db:exec "BEGIN TRANSACTION"

    -- create/reset feature tables

    db:exec [[
    DROP TABLE IF EXISTS feature_move_octant;
    CREATE TABLE IF NOT EXISTS feature_move_octant (
        move_id INTEGER PRIMARY KEY REFERENCES game_moves(id) ON DELETE CASCADE,

        octant TINYINT,
        height TINYINT,
        diaoff TINYINT
    )]]

    db:exec [[
    DROP TABLE IF EXISTS feature_move_last;
    CREATE TABLE feature_move_last (
        move_id INTEGER PRIMARY KEY REFERENCES game_moves(id) ON DELETE CASCADE,

        delta_distsq INTEGER,
        delta_height INTEGER,
        delta_diaoff INTEGER
    )]]

    -- iterate through games
    local stmt_select_games = db:prepare "SELECT id FROM games ORDER BY id ASC"
    while stmt_select_games:step() do
        local game_id = stmt_select_games:get_value(0)
        print(game_id)

        local stmt_select_moves = db:prepare [[
        SELECT id, row, col 
        FROM game_moves 
        WHERE game_id == ? ORDER BY move_num ASC
        ]]

        local stmt_insert_octant = db:prepare [[
        INSERT INTO feature_move_octant (move_id, octant, height, diaoff) 
        VALUES (?, ?, ?, ?)
        ]]

        local stmt_insert_last = db:prepare [[
        INSERT INTO feature_move_last (move_id, delta_distsq, delta_height, delta_diaoff)
        VALUES (?, ?, ?, ?)
        ]]

        local last_row, last_col = nil, nil

        stmt_select_moves:bind(1, game_id)
        while stmt_select_moves:step() do
            local move_id = stmt_select_moves:get_value(0)
            local row = stmt_select_moves:get_value(1)
            local col = stmt_select_moves:get_value(2)

            if row ~= nil and col ~= nil then
                local octant, height, diaoff = kp.octant_from_matrix(row, col, 19)

                do
                    stmt_insert_octant:reset()

                    stmt_insert_octant:bind(1, move_id)
                    stmt_insert_octant:bind(2, octant)
                    stmt_insert_octant:bind(3, height)
                    stmt_insert_octant:bind(4, diaoff)

                    stmt_insert_octant:step()
                end

                if last_row ~= nil and last_col ~= nil then
                    local last_octant, last_height, last_diaoff = kp.octant_from_matrix(last_row, last_col, 19)

                    local delta_distsq = (last_row - row) ^ 2 + (last_col - col) ^ 2
                    local delta_height = tonumber(last_height) - tonumber(height)
                    local delta_diaoff = tonumber(last_diaoff) - tonumber(diaoff)

                    stmt_insert_last:reset()

                    stmt_insert_last:bind(1, move_id)
                    stmt_insert_last:bind(2, delta_distsq)
                    stmt_insert_last:bind(3, delta_height)
                    stmt_insert_last:bind(4, delta_diaoff)

                    stmt_insert_last:step()
                end

                last_row = row
                last_col = col
            end
        end
        stmt_select_moves:finalize()
        stmt_insert_octant:finalize()
        stmt_insert_last:finalize()
    end
    stmt_select_games:finalize()

    db:exec "COMMIT TRANSACTION"
end

return module
