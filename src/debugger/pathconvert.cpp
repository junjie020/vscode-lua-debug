#include <debugger/pathconvert.h>
#include <debugger/impl.h>
#include <debugger/path.h>
#include <base/util/unicode.h>
#include <base/util/dynarray.h>
#include <deque>
#include <Windows.h>

namespace vscode
{
	pathconvert::pathconvert(debugger_impl* dbg)
		: debugger_(dbg)
		, sourcemap_()
		, coding_(coding::ansi)
	{ }

	void pathconvert::set_coding(coding coding)
	{
		coding_ = coding;
		source2client_.clear();
	}

	void pathconvert::add_sourcemap(const std::string& server, const std::string& client)
	{
		sourcemap_.push_back(std::make_pair(server, client));
	}

	void pathconvert::add_skipfiles(const std::string& pattern)
	{
		skipfiles_.push_back(pattern);
	}

	void pathconvert::clear()
	{
		sourcemap_.clear();
	}

	bool pathconvert::match_sourcemap(const std::string& srv, std::string& cli, const std::string& srvmatch, const std::string& climatch)
	{
		size_t i = 0;
		for (; i < srvmatch.size(); ++i) {
			if (i >= srv.size()) {
				return false;
			}
			if (path::tochar(srvmatch[i]) == path::tochar(srv[i])) {
				continue;
			}
			return false;
		}
		cli = climatch + srv.substr(i);
		return true;
	}

	bool pathconvert::server2client(const std::string& server, std::string& client)
	{
		for (auto& it : sourcemap_)
		{
			if (match_sourcemap(server, client, it.first, it.second))
			{
				return true;
			}
		}
		client = path::normalize(server);
		return true;
	}

	bool pathconvert::get(const std::string& source, std::string& client)
	{
		auto it = source2client_.find(source);
		if (it != source2client_.end())
		{
			client = it->second;
			return !client.empty();
		}

		bool res = true;
		if (debugger_->custom_) {
			std::string server;
			if (debugger_->custom_->path_convert(source, server)) {
				res = server2client(server, client);
			}
			else {
				client.clear();
				res = false;
			}
		}
		else {
			if (source[0] == '@') {
				std::string server = coding_ == coding::utf8
					? source.substr(1) 
					: base::a2u(base::strview(source.data() + 1, source.size() - 1))
					;
				res = server2client(server, client);
			}
			else {
				client.clear();
				res = false;
			}
		}
		source2client_[source] = client;
		return res;
	}
}
