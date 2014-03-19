local crypto = require("crypto")

function filter_open(email, page)
        buffer = ""
        hexdigest = crypto.digest("md5", email:sub(2, -2):lower())
end

function filter_close()
        baseurl = os.getenv("HTTPS") and "https://seccdn.libravatar.org/" or "http://cdn.libravatar.org/"
        html("<span class='libravatar'><img class='inline' src='" .. baseurl .. "avatar/" .. hexdigest .. "?s=13&amp;d=retro' /><img class='onhover' src='" .. baseurl .. "avatar/" .. hexdigest .. "?s=128&amp;d=retro' /></span>" .. buffer)
        return 0
end

function filter_write(str)
        buffer = buffer .. str
end
