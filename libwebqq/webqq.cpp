/*
 * Copyright (C) 2012  微蔡 <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "webqq.h"
#include "webqq_impl.h"
#include "avhttp.hpp"
#include "url.hpp"
#include "httpagent.hpp"

webqq::webqq( boost::asio::io_service& asioservice, std::string qqnum, std::string passwd)
	: impl( new qqimpl::WebQQ( asioservice, qqnum, passwd) )
{
}

void webqq::on_verify_code( boost::function< void ( const boost::asio::const_buffer & ) >  cb )
{
	impl->signeedvc.connect( cb );
}


void webqq::on_group_msg( boost::function< void( const std::string, const std::string, const std::vector<qqMsg>& )> cb )
{
	this->impl->siggroupmessage.connect( cb );
}

void webqq::on_group_found( boost::function< void (qqGroup_ptr) > cb )
{
	impl->siggroupnumber.connect(cb);
}

void webqq::on_group_newbee( boost::function<void ( qqGroup_ptr,  qqBuddy * )>  cb )
{
	impl->signewbuddy.connect(cb);
}

static void dummy(){}

void webqq::update_group_member(boost::shared_ptr<qqGroup> group )
{
	impl->update_group_member( group , dummy);
}

qqGroup_ptr webqq::get_Group_by_gid( std::string gid )
{
	return impl->get_Group_by_gid( gid );
}

qqGroup_ptr webqq::get_Group_by_qq( std::string qq )
{
	return impl->get_Group_by_qq( qq );
}

void webqq::login()
{
	impl->login();
}

void webqq::login_withvc( std::string vccode )
{
	impl->login_withvc( vccode );
}


void webqq::send_group_message( std::string group, std::string msg, boost::function<void ( const boost::system::error_code& ec )> donecb )
{
	impl->send_group_message( group, msg, donecb );
}

void webqq::send_group_message( qqGroup& group, std::string msg, boost::function<void ( const boost::system::error_code& ec )> donecb )
{
	impl->send_group_message( group, msg, donecb );
}

boost::asio::io_service& webqq::get_ioservice()
{
	return impl->get_ioservice();
}

bool webqq::is_online()
{
	return impl->m_status == LWQQ_STATUS_ONLINE;
}

static void async_fetch_cface_cb(const boost::system::error_code& ec,
								read_streamptr stream,  boost::asio::streambuf & buf,
								boost::function<void(boost::system::error_code ec, boost::asio::streambuf & buf)> callback)
{
// store result to cface_data
	callback(ec, buf);
}

void webqq::async_fetch_cface_std_saver( boost::system::error_code ec, boost::asio::streambuf& buf, std::string cface, boost::filesystem::path parent_path)
{
	if (!fs::exists(parent_path)){
		fs::create_directories(parent_path);
	}

	if (!ec || ec == boost::asio::error::eof){
		std::string imgfilename = (parent_path / cface).string();
		std::ofstream cfaceimg(imgfilename.c_str(), std::ofstream::binary|std::ofstream::out);
		cfaceimg.write(boost::asio::buffer_cast<const char*>(buf.data()), boost::asio::buffer_size(buf.data()));
	}
}

void webqq::async_fetch_cface(boost::asio::io_service & io_service, const qqMsgCface & cface, boost::function<void(boost::system::error_code ec, boost::asio::streambuf & buf)> callback)
{
	std::string url = boost::str( 
						boost::format( "http://web.qq.com/cgi-bin/get_group_pic?gid=%s&uin=%s&fid=%s&pic=%s&vfwebqq=%s" )
						% cface.gid
						% cface.uin
						% cface.file_id
						% url_encode( cface.name )
						% cface.vfwebqq
					);

	read_streamptr stream;
	stream.reset( new avhttp::http_stream( io_service ) );
	stream->request_options(
		avhttp::request_opts()
			(avhttp::http_options::cookie, cface.cookie)
	);
	async_http_download( stream, url, boost::bind(async_fetch_cface_cb, _1, _2, _3, callback));
}

static void async_cface_url_final_cb(const boost::system::error_code& ec,
								read_streamptr stream, boost::function<void(boost::system::error_code ec, std::string)> callback)
{
	callback(ec, stream->location());
}

void webqq::async_cface_url_final(boost::asio::io_service & io_service, const qqMsgCface & cface, boost::function<void(boost::system::error_code ec, std::string)> callback)
{
	std::string url = boost::str(
						boost::format( "http://web.qq.com/cgi-bin/get_group_pic?gid=%s&uin=%s&fid=%s&pic=%s&vfwebqq=%s" )
						% cface.gid
						% cface.uin
						% cface.file_id
						% url_encode( cface.name )
						% cface.vfwebqq
					);

	read_streamptr stream;
	stream.reset( new avhttp::http_stream( io_service ) );
	stream->request_options(
		avhttp::request_opts()
			(avhttp::http_options::cookie, cface.cookie)
	);

	stream->max_redirects(0);
	stream->async_open(url, boost::bind(async_cface_url_final_cb, _1, stream, callback));
}

void webqq::search_group( std::string groupqqnum, std::string vfcode, webqq::search_group_handler handler )
{
	impl->search_group(groupqqnum, vfcode, handler);
}

void webqq::join_group( qqGroup_ptr group, std::string vfcode, webqq::join_group_handler handler )
{
	impl->join_group(group, vfcode, handler);
}
