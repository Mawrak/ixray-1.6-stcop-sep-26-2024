#include "stdafx.h"
#include "ContentView.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <StbImage/stb_image.h>
#include <StbImage/stb_image_resize.h>

CContentView::CContentView():
	WatcherPtr(nullptr)
{
	string_path Dir = {};
	FS.update_path(Dir, "$fs_root$", "");

	RootDir = std::filesystem::path(Dir).string().data();
	CurrentDir = RootDir;

	FS.update_path(Dir, "$logs$", "");
	LogsDir = Dir;
}

void CContentView::Draw()
{
	if (ImGui::Begin("Content Browser"))
	{
		if (Files.empty())
		{
			RescanDirectory();
		}

		while (LockFiles)
			continue;

		LockFiles = true;

		const size_t IterCount = (ImGui::GetWindowSize().x / (BtnSize.x + 15)) - 1;
		size_t HorBtnIter = 0;
		xr_string NextDir = CurrentDir;

		if (!RootDir.Contains(CurrentDir) && !IsSpawnElement)
		{
			DrawOtherDir(HorBtnIter, IterCount, NextDir);
		}
		else if (IsSpawnElement)
		{
			DrawISEDir(HorBtnIter, IterCount);
		}
		else
		{
			DrawRootDir(HorBtnIter, IterCount, NextDir);
		}

		LockFiles = false;
		CurrentDir = NextDir;
		xr_strlwr(CurrentDir);
	}

	if (CurrentItemHint.Active)
	{
		ImGui::SetCursorPos(CurrentItemHint.Pos);
		ImGui::Button(CurrentItemHint.Name.c_str());
		CurrentItemHint.Active = false;
	}

	ImGui::End();
}

void CContentView::DrawISEDir(size_t& HorBtnIter, const size_t IterCount)
{
	if (DrawItem({ "..", true }, HorBtnIter, IterCount))
	{
		if (ISEPath.empty())
		{
			IsSpawnElement = false;
			ISEPath = "";
			Files.clear();
		}
		else
		{
			xr_string Validate = ISEPath;
			if (Validate.ends_with('\\'))
			{
				Validate = Validate.erase(Validate.length() - 1);
			}
			std::filesystem::path ISEFS = Validate.data();

			if (ISEFS.has_parent_path())
			{
				RescanISEDirectory(ISEFS.parent_path().string().data());
				ISEPath = ISEFS.parent_path().string().data();
			}
			else
			{
				RescanISEDirectory("");
				ISEPath = "";
			}
		}
	}

	for (const FileOptData& Data : Files)
	{
		if (DrawItem(Data, HorBtnIter, IterCount))
		{
			if (Data.IsDir)
			{
				xr_string CopyFileName = Data.File.string().c_str();
				RescanISEDirectory(CopyFileName);
			}

			break;
		}
	}
}

void CContentView::DrawRootDir(size_t& HorBtnIter, const size_t& IterCount, xr_string& NextDir)
{
	string_path FSEntry = {};
	auto PathClickLambda = [&FSEntry, &HorBtnIter, &IterCount, &NextDir, this]()
	{
		xr_string Validate = FSEntry;
		if (Validate.ends_with('\\'))
		{
			Validate = Validate.erase(Validate.length() - 1);
		}

		if (DrawItem({ Validate.c_str(), true }, HorBtnIter, IterCount))
		{
			NextDir = FSEntry;
			if (NextDir.ends_with('\\'))
			{
				NextDir = NextDir.erase(NextDir.length() - 1);
			}
			Files.clear();
		}
	};

	IsSpawnElement = false;

	FS.update_path(FSEntry, "$server_data_root$", "");
	PathClickLambda();

	FS.update_path(FSEntry, "$import$", "");
	PathClickLambda();

	FS.update_path(FSEntry, "$game_data$", "");
	PathClickLambda();

	if (DrawItem({ "Spawn Elements", true }, HorBtnIter, IterCount))
	{
		RescanISEDirectory("");
	}
}

void CContentView::RescanISEDirectory(const xr_string& StartPath)
{
	Files.clear();

	if (!StartPath.empty())
	{
		ISEPath += StartPath + '\\';
	}

	xr_map<xr_string, FileOptData> TempPath;
	CInifile::Root& data = ((CInifile*)pSettings)->sections();

	for (CInifile::RootIt it = data.begin(); it != data.end(); it++) 
	{
		LPCSTR val;
		if ((*it)->line_exist("$spawn", &val))
		{
			shared_str caption = pSettings->r_string_wb((*it)->Name, "$spawn");
			shared_str sect = (*it)->Name;
			if (caption.size())
			{
				xr_string FileName = caption.c_str();

				if (!FileName.Contains(StartPath) && !StartPath.empty())
					continue;

				if (FileName == StartPath)
					continue;

				if (StartPath.empty())
				{
					size_t DirStart = FileName.find('\\');

					if (DirStart != xr_string::npos)
					{
						xr_string DirName = FileName.substr(0, DirStart);
						if (TempPath.contains(DirName))
							continue;

						TempPath[DirName] = { DirName.c_str(), true };
						continue;
					}
				}
				else
				{
					const xr_string Delimer = StartPath + '\\';

					size_t DirStart = FileName.find(Delimer);

					if (DirStart != xr_string::npos)
					{
						xr_string DirName = FileName.substr(DirStart + Delimer.length());
						if (TempPath.contains(DirName))
							continue;

						if (DirName.Contains("\\"))
						{
							TempPath[DirName] = { DirName.c_str(), true };
						}
						else
						{
							TempPath[DirName] = { (DirName + ".ise").c_str(), false, sect };
						}
					}

					continue;
				}

				TempPath[FileName] = { (FileName + ".ise").c_str(), false, sect };
			}
		}
	}

	for (auto& [Name, DirOpt] : TempPath)
	{
		if (DirOpt.IsDir)
		{
			Files.push_back(DirOpt);
		}
	}

	for (auto& [Name, DirOpt] : TempPath)
	{
		if (!DirOpt.IsDir)
		{
			Files.push_back(DirOpt);
		}
	}

	IsSpawnElement = true;
}

void CContentView::DrawOtherDir(size_t& HorBtnIter, const size_t IterCount, xr_string& NextDir)
{
	std::filesystem::path FilePath = CurrentDir.c_str();
	if (DrawItem({ "..", true }, HorBtnIter, IterCount))
	{
		NextDir = FilePath.parent_path().string().data();
		if (FilePath.parent_path().is_absolute() && !NextDir.Contains(RootDir) || NextDir.empty())
		{
			NextDir = RootDir;
		}
		Files.clear();
	}

	for (const FileOptData& FilePath : Files)
	{
		if (FilePath.IsDir)
		{
			if (DrawItem(FilePath, HorBtnIter, IterCount))
			{
				NextDir = FilePath.File.string().data();
				if (NextDir.ends_with('\\'))
				{
					NextDir = NextDir.erase(NextDir.length() - 1);
				}
				Files.clear();
				break;
			}
		}
		else
		{
			if (DrawItem(FilePath, HorBtnIter, IterCount))
			{
				if (FilePath.File.extension() == ".tga")
				{
					string_path fn = {};
					FS.update_path(fn, _textures_, "");
					xr_string OldPath = FilePath.File.string().data();

					auto CharIndex = OldPath.find(fn);
					if (CharIndex != xr_string::npos)
					{
						xr_string NewPath = OldPath.substr(OldPath.find(fn) + xr_strlen(fn));
						NewPath = NewPath.substr(0, NewPath.find_last_of("."));
						ExecCommand(COMMAND_IMAGE_EDITOR_SELECT, NewPath, false);
					}
					else
					{
						FS.update_path(fn, _import_, "");
						CharIndex = OldPath.find(fn);

						if (CharIndex != xr_string::npos)
						{
							xr_string NewPath = OldPath.substr(OldPath.find(fn) + xr_strlen(fn));
							ExecCommand(COMMAND_IMAGE_EDITOR_SELECT, NewPath, true);
						}
					}
				}
			}
		}
	}
}

void CContentView::RescanDirectory()
{
	IsDelWatcher = true;
	xr_delete(WatcherPtr);

	for (const auto& file : std::filesystem::directory_iterator{ CurrentDir.data() })
	{
		if (std::filesystem::is_directory(file))
		{
			Files.push_back({ file, true });
		}
	}
	for (const std::filesystem::path& file : std::filesystem::directory_iterator{ CurrentDir.data() })
	{
		if (!std::filesystem::is_directory(file) && file.has_extension() && file.extension().string() != ".thm")
		{
			Files.push_back({ file, false });
		}
	}

	WatcherPtr = new filewatch::FileWatch<std::string>
	(
		CurrentDir.data(),
		[this](const std::string&, const filewatch::Event)
		{
			while (LockFiles || IsSpawnElement)
			{
				if (IsDelWatcher)
					return;

				continue;
			}

			LockFiles = true;
			Files.clear();
			LockFiles = false;
		}
	);

	IsDelWatcher = false;
}

void CContentView::Destroy()
{
 	Icons.clear();
}

void CContentView::ResetBegin() {
}

void CContentView::ResetEnd() {
}

void CContentView::Init()
{
	Icons["Folder"] = {EDevice->Resources->_CreateTexture("ed\\content_browser\\folder"),	true};
	Icons[".."]		= {EDevice->Resources->_CreateTexture("ed\\content_browser\\folder"),	true};
	Icons["thm"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\thm"),		true};
	Icons["logs"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\log"),		true};
	Icons["ogg"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\ogg"),		true};
	Icons["wav"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\wav"),		true};
	Icons["object"] = {EDevice->Resources->_CreateTexture("ed\\content_browser\\object"),	true};
	Icons["image"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\image"),	true};
	Icons["seq"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\seq"),		true};
	Icons["tga"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\tga"),		true};
	Icons["file"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\file"),		true};
	Icons["exe"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\exe"),		true};
	Icons["cmd"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\cmd"),		true};
	Icons["dll"]	= {EDevice->Resources->_CreateTexture("ed\\content_browser\\dll"),		true};
	Icons["backup"] = {EDevice->Resources->_CreateTexture("ed\\content_browser\\backup"),	true};
}

bool CContentView::DrawItem(const FileOptData& InitFileName, size_t& HorBtnIter, const size_t IterCount)
{
	std::filesystem::path FilePath = InitFileName.File.c_str();
	const ImVec2& CursorPos = ImGui::GetCursorPos();

	xr_string FileName = FilePath.filename().string().data();
	IconData* IconPtr = nullptr;

	bool OutValue = false;
	if (Contains())
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		IconPtr = InitFileName.IsDir ? &GetTexture("Folder") : &GetTexture(FilePath.string().data());
		ImVec4 IconColor = IconPtr->UseButtonColor ? colors[ImGuiCol_CheckMark] : ImVec4(1, 1, 1, 1);

		if (!IconPtr->Icon)
			return false;

		OutValue = ImGui::ImageButton
		(
			FileName.c_str(),
			IconPtr->Icon->pSurface, BtnSize,
			ImVec2(0, 0), ImVec2(1, 1),
			ImVec4(0, 0, 0, 0), IconColor
		);
	}
	else
	{
		ImGui::Button(FileName.c_str(), BtnSize);
		ImGui::Text(FileName.c_str());

		if (HorBtnIter != IterCount)
		{
			ImGui::SetCursorPosY(CursorPos.y);
			ImGui::SetCursorPosX(CursorPos.x + 15 + BtnSize.x);
			HorBtnIter++;
		}
		else
		{
			HorBtnIter = 0;
		}

		return false;
	}

	if (!DrawContext(FilePath))
	{
		if (ImGui::IsItemHovered())
		{
			ImVec2 DrawHintPos = ImGui::GetMousePos() - ImGui::GetWindowPos() + ImVec2{ ImGui::GetScrollX(), ImGui::GetScrollY()};
			DrawHintPos.y -= 15;
			CurrentItemHint = { Platform::ANSI_TO_UTF8(FileName) ,DrawHintPos, true };
		}
	}

	xr_string LabelText = FilePath.has_extension() ? FileName.substr(0, FileName.length() - FilePath.extension().string().length()).c_str() : FileName.c_str();

	if (ImGui::BeginDragDropSource())
	{
		Data.FileName = InitFileName.ISESect.c_str();
		ImGui::SetDragDropPayload("TEST", &Data, sizeof(DragDropData));
		ImGui::ImageButton(FilePath.filename().string().c_str(), IconPtr->Icon->pSurface, BtnSize);
		ImGui::Text(LabelText.data());
		ImGui::EndDragDropSource();
	}
	else
	{
		float TextPixels = ImGui::CalcTextSize(LabelText.data()).x;

		while (TextPixels > BtnSize.x)
		{
			LabelText = LabelText.substr(0, LabelText.length() - 4) + "..";
			TextPixels = ImGui::CalcTextSize(LabelText.data()).x;
		}

		ImGui::SetCursorPosX(CursorPos.x + (((10 + BtnSize.x) - TextPixels) / 2));

		ImGui::Text(Platform::ANSI_TO_UTF8(LabelText).data());
	}

	if (HorBtnIter != IterCount)
	{
		ImGui::SetCursorPosY(CursorPos.y);
		ImGui::SetCursorPosX(CursorPos.x + 15 + BtnSize.x);
		HorBtnIter++;
	}
	else
	{
		HorBtnIter = 0;
	}
	return OutValue;
}

bool CContentView::Contains()
{
	float ScrollValue = ImGui::GetScrollY();
	float CursorPosY = ImGui::GetCursorPosY();

	bool IsNotAfter = CursorPosY < ScrollValue + ImGui::GetWindowSize().y;
	bool IsNotBefor = CursorPosY > ScrollValue - BtnSize.y;
	return IsNotAfter && IsNotBefor;
}

bool CContentView::DrawContext(const std::filesystem::path& Path) const
{
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Delete"))
		{
			std::filesystem::remove(Path);
			FS.rescan_path(Path.parent_path().string().c_str() , true);
		}
		ImGui::EndPopup();
		return true;
	}

	return false;
}

CContentView::IconData & CContentView::GetTexture(const xr_string & IconPath)
{
	if (IconPath.find(".~") != xr_string::npos)
		return Icons["backup"];

	if (IconPath.ends_with(".ltx"))
		return Icons["thm"];
	
	if (IconPath.ends_with(".ogg"))
		return Icons["ogg"];
	
	if (IconPath.ends_with(".wav"))
		return Icons["wav"];

	if (IconPath.ends_with(".seq"))
		return Icons["seq"];

	if (IconPath.ends_with(".dll"))
		return Icons["dll"];

	if (IconPath.ends_with(".exe"))
		return Icons["exe"];

	if (IconPath.ends_with("$") && IconPath.starts_with("$"))
		return Icons["Folder"];

	if (IconPath.ends_with(".cmd") ||
		IconPath.ends_with(".bat"))
		return Icons["cmd"];
	
	if (IconPath.Contains(LogsDir))
		return Icons["logs"];

	if (!Icons.contains(IconPath))
	{
		if (IconPath.ends_with(".ise"))
		{
			ESceneSpawnTool* SpTool = (ESceneSpawnTool*)Scene->GetTool(OBJCLASS_SPAWNPOINT);
			xr_string ValidPath = IconPath;
			ValidPath = ValidPath.erase(ValidPath.length() - 4);

			if (pSettings->line_exist(ValidPath.data(), "$ed_icon"))
			{
				Icons[IconPath] = { EDevice->Resources->_CreateTexture(pSettings->r_string_wb(ValidPath.data(), "$ed_icon").c_str()), false };
				Icons[IconPath].Icon->Load();
			}
			else
			{
				Icons[IconPath] = Icons["file"];
			}
		}
		else if (IconPath.ends_with(".object"))
		{
			string_path fn = {};
			FS.update_path(fn, _objects_, fn);
			Icons[IconPath] = Icons["object"];

			if(IconPath.find(fn) != xr_string::npos) {
				xr_string NewPath = IconPath.substr(IconPath.find(fn) + xr_strlen(fn));

				EObjectThumbnail* m_Thm = (EObjectThumbnail*)ImageLib.CreateThumbnail(NewPath.data(), EImageThumbnail::ETObject);
				CTexture* TempTexture = new CTexture();
				m_Thm->Update(TempTexture->pSurface);

				if(TempTexture->pSurface != nullptr) {
					Icons[IconPath] = {TempTexture, false};
				}
				else {
					xr_delete(TempTexture);
				}
			}
		}
		else if (IconPath.ends_with(".group"))
		{
			string_path fn = {};
			FS.update_path(fn, _groups_, "");
			Icons[IconPath] = Icons["object"];

			if (IconPath.find(fn) != xr_string::npos) {
				xr_string NewPath = IconPath.substr(IconPath.find(fn) + xr_strlen(fn));

				EGroupThumbnail* m_Thm = xr_new<EGroupThumbnail>(NewPath.data());
				//EObjectThumbnail* m_Thm = (EObjectThumbnail*)ImageLib.CreateThumbnail(NewPath.data(), EImageThumbnail::ETTexture);
				CTexture* TempTexture = new CTexture();
				m_Thm->Update(TempTexture->pSurface);

				if (TempTexture->pSurface != nullptr) {
					Icons[IconPath] = { TempTexture, false };
				}
				else {
					xr_delete(TempTexture);
				}
			}
		}
		else if(IconPath.ends_with(".tga"))
		{
			string_path fn = {};
			FS.update_path(fn, _textures_, "");
			Icons[IconPath] = Icons["tga"];

			if(IconPath.find(fn) != xr_string::npos) {
				xr_string NewPath = IconPath.substr(IconPath.find(fn) + xr_strlen(fn));

				EObjectThumbnail* m_Thm = (EObjectThumbnail*)ImageLib.CreateThumbnail(NewPath.data(), EImageThumbnail::ETTexture);
				CTexture* TempTexture = new CTexture();
				m_Thm->Update(TempTexture->pSurface);

				if(TempTexture->pSurface != nullptr) {
					Icons[IconPath] = {TempTexture, false};
				}
				else {
					xr_delete(TempTexture);
				}
			}
		}
		else if (IconPath.ends_with(".png") || IconPath.ends_with(".tga"))
		{
			U8Vec Pixels;
			int w, h, a;
			stbi_uc* raw_data = stbi_load((LPSTR)IconPath.c_str(), &w, &h, &a, STBI_rgb_alpha);
			Pixels.resize(50 * 50 * a);
			if (raw_data != nullptr)
			{
				stbir_resize_uint8(raw_data, w, h, 0, Pixels.data(), 50, 50, 0, a);
				CTexture* TempTexture = new CTexture();
				ID3DTexture2D* pTexture = nullptr;
				Icons[IconPath] = { TempTexture, false };
				R_CHK(REDevice->CreateTexture(50, 50, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, 0));
				{
					D3DLOCKED_RECT rect;
					R_CHK(pTexture->LockRect(0, &rect, 0, D3DLOCK_DISCARD));
					memcpy(rect.pBits, Pixels.data(), 50 * 50 * a);
					R_CHK(pTexture->UnlockRect(0));

					TempTexture->pSurface = pTexture;
				}

				stbi_image_free(raw_data);
			}
		}
		else if(IconPath.ends_with(".dds")) 
		{
			xr_string NewPath = IconPath.substr(0, IconPath.length() - 4);

			Icons[IconPath] = {EDevice->Resources->_CreateTexture(NewPath.c_str()), false};
			Icons[IconPath].Icon->Load();

			if(!Icons[IconPath].Icon->pSurface) {
				Icons[IconPath] = Icons["image"];
			}
		}
		else
		{
			Icons[IconPath] = Icons["file"];
		}
	}

	return Icons[IconPath];
}
