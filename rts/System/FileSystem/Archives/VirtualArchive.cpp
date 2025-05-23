/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VirtualArchive.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Log/ILog.h"

#include "zlib.h"
#include "minizip/zip.h"
#include <cassert>

CVirtualArchiveFactory* virtualArchiveFactory;

CVirtualArchiveFactory::CVirtualArchiveFactory() : IArchiveFactory("sva")
{
	virtualArchiveFactory = this;
}

CVirtualArchiveFactory::~CVirtualArchiveFactory()
{
	virtualArchiveFactory = nullptr;
}


CVirtualArchive* CVirtualArchiveFactory::AddArchive(const std::string& fileName)
{
	CVirtualArchive* archive = new CVirtualArchive(fileName);
	archives.push_back(archive);
	return archive;
}

IArchive* CVirtualArchiveFactory::DoCreateArchive(const std::string& fileName) const
{
	const std::string baseName = FileSystem::GetBasename(fileName);

	for (CVirtualArchive* archive: archives) {
		if (archive->GetFileName() == baseName)
			return archive->Open();
	}

	return nullptr;
}

CVirtualArchiveOpen::CVirtualArchiveOpen(CVirtualArchive* archive, const std::string& fileName)
	: IArchive(fileName)
	, archive(archive)
{
	// set subclass name index to archive's index (doesn't update while archive is open)
	lcNameIndex = archive->GetNameIndex();
}


uint32_t CVirtualArchiveOpen::NumFiles() const
{
	return archive->NumFiles();
}

bool CVirtualArchiveOpen::GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	return archive->GetFile(fid, buffer);
}

const std::string& CVirtualArchiveOpen::FileName(uint32_t fid) const
{
	return archive->FileName(fid);
}

int32_t CVirtualArchiveOpen::FileSize(uint32_t fid) const
{
	return archive->FileSize(fid);
}

IArchive::SFileInfo CVirtualArchiveOpen::FileInfo(uint32_t fid) const
{
	return archive->FileInfo(fid);
}



CVirtualArchiveOpen* CVirtualArchive::Open()
{
	return new CVirtualArchiveOpen(this, fileName);
}


bool CVirtualArchive::GetFile(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	if (fid >= files.size())
		return false;

	buffer = files[fid].buffer;
	return true;
}

const std::string& CVirtualArchive::FileName(uint32_t fid) const
{
	assert(fid < files.size());
	return files[fid].name;
}

int32_t CVirtualArchive::FileSize(uint32_t fid) const
{
	assert(fid < files.size());
	return static_cast<int32_t>(files[fid].buffer.size());
}

IArchive::SFileInfo CVirtualArchive::FileInfo(uint32_t fid) const
{
	assert(fid < files.size());
	const auto& fe = files[fid];
	return IArchive::SFileInfo{
		.fileName = fe.name,
		.specialFileName = "",
		.size = static_cast<int32_t>(fe.buffer.size()),
		.modTime = 0
	};
}

uint32_t CVirtualArchive::AddFile(const std::string& name)
{
	lcNameIndex[name] = files.size();
	files.emplace_back(files.size(), name);

	return (files.size() - 1);
}

void CVirtualArchive::WriteToFile()
{
	const std::string zipFilePath = dataDirsAccess.LocateFile(fileName, FileQueryFlags::WRITE) + ".sdz";
	LOG("Writing zip file for virtual archive %s to %s", fileName.c_str(), zipFilePath.c_str());

	zipFile zip = zipOpen(zipFilePath.c_str(), APPEND_STATUS_CREATE);

	if (zip == nullptr) {
		LOG("[VirtualArchive::%s] could not open zip file %s for writing", __func__, zipFilePath.c_str());
		return;
	}

	for (const CVirtualFile& file: files) {
		file.WriteZip(zip);
	}

	zipClose(zip, nullptr);
}

void CVirtualFile::WriteZip(void* zf) const
{
	zipFile zip = static_cast<zipFile>(zf);

	zipOpenNewFileInZip(zip, name.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION);
	zipWriteInFileInZip(zip, buffer.data(), buffer.size());
	zipCloseFileInZip(zip);
}

