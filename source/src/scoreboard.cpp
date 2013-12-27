// creation of scoreboard pseudo-menu

#include "pch.h"
#include "cube.h"

void *scoremenu = NULL, *teammenu = NULL, *ctfmenu = NULL;

void showscores(int on){
	if(on) showmenu(m_affinity(gamemode) ? "flag score" : (m_team(gamemode, mutators) ? "team score" : "score"), false);
	else if (!intermission){
		closemenu("score");
		closemenu("team score");
		closemenu("flag score");
	}
}

void showscores_(char *on){
	if(on[0]) showscores(ATOI(on));
	else showscores(addreleaseaction("showscores")!=NULL);
}

COMMANDN(showscores, showscores_, ARG_1STR);

struct sline{
	string s;
	const char *altfont;
	color *bgcolor;
	sline() : altfont(NULL), bgcolor(NULL) { *s = '\0'; }
};

static vector<sline> scorelines;

teamscore teamscores[TEAM_NUM] = { teamscore(TEAM_RED), teamscore(TEAM_BLUE), teamscore(TEAM_SPECT) };

struct teamsum{
	int team, lvl, ping, pj;
	vector<playerent *> teammembers;
	teamsum(int t) : team(t), lvl(0), ping(0), pj(0) {}

	virtual void addscore(playerent *d){
		if(!d) return;
		teammembers.add(d);
		extern int level;
		lvl += d == player1 ? level : d->level;
		ping += d->ping;
		pj += d->plag;
	}
};

#define doscorecompare(x, y) \
	if((x)->flagscore > (y)->flagscore) return -1; \
	if((x)->flagscore < (y)->flagscore) return 1; \
	if((x)->points > (y)->points) return -1; \
	if((x)->points < (y)->points) return 1; \
	if((x)->frags > (y)->frags) return -1; \
	if((x)->frags < (y)->frags) return 1; \
	if((x)->assists > (y)->assists) return -1; \
	if((x)->assists < (y)->assists) return 1; \
	if((x)->deaths < (y)->deaths) return -1; /* less is better */ \
	if((x)->deaths > (y)->deaths) return 1;

static int teamscorecmp(const teamsum *a, const teamsum *b){
	teamscore *tsa = &teamscores[a->team], *tsb = &teamscores[b->team];
	doscorecompare(tsa, tsb)
	return a->team > b->team;
}

static int scorecmp(const playerent **a, const playerent **b){
	doscorecompare(*a, *b)
	return strcmp((**a).name, (**b).name);
}

static int pointcmp(const playerent **a, const playerent **b){
	const playerent *x = *a, *y = *b;
	if(x->points > y->points) return -1;
	if(x->points < y->points) return 1;
	return 0;
}

struct scoreratio{
	float ratio;
	int precision;

	void calc(int frags, int deaths){
		// ratio
		if(frags>=0 && deaths>0) ratio = (float)frags/(float)deaths;
		else if(frags>=0 && deaths==0) ratio = frags * 2.f;
		else ratio = 0.0f;

		// precision
		if(ratio<10.0f) precision = 2;
		else if(ratio>=10.0f && ratio<100.0f) precision = 1;
		else precision = 0;
	}
};

void renderscore(void *menu, playerent *d){
	static color localplayerc(0.2f, 0.2f, 0.2f, 0.2f), damagedplayerc(0.4f, 0.1f, 0.1f, 0.3f);
	string lagping;
	if(d->team == TEAM_SPECT) formatstring(lagping)("%s", colorping(d->ping));
	else formatstring(lagping)("%s/%s", d->state==CS_WAITING ? "LAG" : colorpj(d->plag), colorping(d->ping));
	int rmod10 = d->rank % 10;
	defformatstring(rankstr)("%d%s", d->rank, (d->rank / 10 == 1) ? "th" : rmod10 == 1 ? "st" : rmod10 == 2 ? "nd" : rmod10 == 3 ? "rd" : "th");
	sline &line = scorelines.add();
	line.bgcolor = d->lastpain + 500 > lastmillis ? &damagedplayerc : d==player1 ? &localplayerc : NULL;
	static scoreratio sr;
	sr.calc(d->frags, d->deaths);
	formatstring(line.s)("%d\t", d->points);
	if(m_affinity(gamemode)) concatformatstring(line.s, "%d\t", d->flagscore);
	extern int level;
	concatformatstring(line.s, "%d\t%d\t%d\t%.*f\t%s\t%s\t%d\t\f%d%s ", d->frags, d->assists, d->deaths, sr.precision, sr.ratio, lagping, rankstr, d == player1 ? level : d->level, privcolor(d->priv, d->state == CS_DEAD), colorname(d, true));
	line.altfont = "build";
	const int buildinfo = d->build | (d == player1 ? getbuildtype() : 0), third = (d == player1) ? thirdperson : d->thirdperson;
	if(d->ownernum >= 0); // bot icon? in the future?
	else if(buildinfo & 0x40) concatstring(line.s, "\a4  "); // Windows
	else if(buildinfo & 0x20) concatstring(line.s, "\a3  "); // Mac
	else if(buildinfo & 0x04) concatstring(line.s, "\a2  "); // Linux
	if(buildinfo & 0x08) concatstring(line.s, "\a1  "); // Debug
	if(third) concatstring(line.s, "\a0  "); // Third-Person
	if(buildinfo & 0x02) concatstring(line.s, "\a5  "); // Authed
	if(d->ignored) concatstring(line.s, "\a6  "); // Muted
}

void renderteamscore(void *menu, teamsum &t){
	if(!scorelines.empty()){ // space between teams
		sline &space = scorelines.add();
		space.s[0] = 0;
	}
	sline &line = scorelines.add();
	defformatstring(plrs)("(%d %s)", t.teammembers.length(), t.team == TEAM_SPECT ? _("sb_spectating") :
															m_zombie(gamemode) && t.team == TEAM_RED ? _("sb_zombies") :
															t.teammembers.length() == 1 ? _("sb_player") : _("sb_players"));
	const teamscore &ts = teamscores[t.team];
	static scoreratio sr;
	string lagping;
	sr.calc(ts.frags, ts.deaths);
	if(t.team == TEAM_SPECT) formatstring(lagping)("%s", colorping(t.ping/max(t.teammembers.length(), 1)));
	else formatstring(lagping)("%s/%s", colorpj(t.pj/max(t.teammembers.length(),1)), colorping(t.ping/max(t.teammembers.length(), 1)));
	const char *teamname = m_team(gamemode, mutators) || t.team == TEAM_SPECT ? team_string(t.team) : "FFA Total";
	formatstring(line.s)("%d\t", ts.points);
	if(m_affinity(gamemode)) concatformatstring(line.s, "%d\t", ts.flagscore);
	concatformatstring(line.s, "%d\t%d\t%d\t%.*f\t%s\t\t%d\t%s\t\t%s", ts.frags, ts.assists, ts.deaths, sr.precision, sr.ratio, lagping, t.lvl, teamname, plrs);
	static color teamcolors[TEAM_NUM+1] = { color(1.0f, 0, 0, 0.2f), color(0, 0, 1.0f, 0.2f), color(.4f, .4f, .4f, .3f), color(.8f, .8f, .8f, .4f) };
	line.bgcolor = &teamcolors[!m_team(gamemode, mutators) && t.team != TEAM_SPECT ? TEAM_NUM : t.team];
	loopv(t.teammembers){
		if(m_zombie(gamemode) && t.teammembers[i]->team == TEAM_RED && t.teammembers[i]->ownernum >= 0 && t.teammembers[i]->state == CS_DEAD) continue;
		renderscore(menu, t.teammembers[i]);
	}
}

extern bool watchingdemo;

void renderscores(void *menu, bool init){
	static string modeline, serverline;

	modeline[0] = '\0';
	serverline[0] = '\0';
	scorelines.shrink(0);

	vector<playerent *> scores;
	scores.add(player1);
	loopv(players) if(players[i]) scores.add(players[i]);

	// rank players
	scores.sort(pointcmp);
	int n = 0;
	loopv(scores){
		// same as previous
		if(i <= 0 || scores[i-1]->points != scores[i]->points) ++n;
		scores[i]->rank = n;
	}
	// sort for the scoreboard
	scores.sort(scorecmp);

	if(init){
		int sel = scores.find(player1);
		if(sel>=0) menuselect(menu, sel);
	}

	if(getclientmap()[0]){
		bool fldrprefix = !strncmp(getclientmap(), "maps/", strlen("maps/"));
		formatstring(modeline)("\"%s\" on map %s", modestr(gamemode, mutators, modeacronyms > 0), fldrprefix ? getclientmap()+strlen("maps/") : getclientmap());
	}

	extern int minutesremaining, gametimecurrent, lastgametimeupdate, gametimemaximum;
	if(!minutesremaining) concatstring(modeline, ", intermission");
	else if(minutesremaining > 0){
		const int cssec = (gametimemaximum-gametimecurrent-(lastmillis-lastgametimeupdate))/1000;
		defformatstring(timestr)(", %d:%02d remaining", (int)floor(cssec/60.f), cssec%60);
		concatstring(modeline, timestr);
	}
	else{
		const int cssec = (gametimecurrent+(lastmillis-lastgametimeupdate))/1000;
		defformatstring(timestr)(", %d:%02d elasped", (int)floor(cssec/60.f), cssec%60);
		concatstring(modeline, timestr);
	}

	if(multiplayer(false)){
		serverinfo *s = getconnectedserverinfo();
		if(s) formatstring(serverline)("%s:%d %s", s->name, s->port, s->sdesc);
	}

	//if(m_team(gamemode, mutators)){
		teamsum teamsums[TEAM_NUM] = { teamsum(TEAM_RED), teamsum(TEAM_BLUE), teamsum(TEAM_SPECT) };

		#define fixteam(pl) (pl->team == TEAM_SPECT ? TEAM_SPECT : m_team(gamemode, mutators) ? pl->team : TEAM_RED)
		loopv(scores) teamsums[fixteam(scores[i])].addscore(scores[i]);

		loopi(TEAM_NUM) teamsums[i].teammembers.sort(scorecmp);
		int sort = teamscorecmp(&teamsums[TEAM_RED], &teamsums[TEAM_BLUE]);
		if(!m_team(gamemode, mutators)) renderteamscore(menu, teamsums[0]);
		else loopi(2) renderteamscore(menu, teamsums[sort < 0 ? i : i^1]);
		if(teamsums[TEAM_SPECT].teammembers.length()) renderteamscore(menu, teamsums[TEAM_SPECT]);
	//}
	//else loopv(scores) renderscore(menu, scores[i]);

	menureset(menu);
	loopv(scorelines) menuimagemanual(menu, NULL, scorelines[i].altfont, scorelines[i].s, NULL, scorelines[i].bgcolor);
	menuheader(menu, modeline, serverline);

	// update server stats
	static int lastrefresh = 0;
	if(!lastrefresh || lastrefresh+5000<lastmillis){
		refreshservers(NULL, init);
		lastrefresh = lastmillis;
	}
}

void consolescores(){
	static vector<playerent *> scores;
	scores.setsize(0);
	if(!watchingdemo) scores.add(player1);
	loopv(players) if(players[i]) scores.add(players[i]);
	scores.sort(scorecmp);

	if(getclientmap()[0]) printf("\n\"%s\" on map %s", modestr(gamemode, mutators), getclientmap());
	if(multiplayer(false)){
		serverinfo *s = getconnectedserverinfo();
		if(s){
			string text;
			filtertext(text, s->sdesc, 1);
			printf(", %s:%d %s", s->name, s->port, text);
		}
	}
	printf("\npoints %sfrags assists deaths ratio cn%s name\n", m_affinity(gamemode) ? "flags " : "", m_team(gamemode, mutators) ? " team" : "");
	loopv(scores){
		const playerent * const d = scores[i];
		scoreratio sr;
		sr.calc(d->frags, d->deaths);
		static string team, flags;
		formatstring(team)(" %-4s", team_string(d->team));
		formatstring(flags)(" %4d ", d->flagscore);
		printf("%6d %s %4d %7d   %4d %5.2f %2d%s %s%s\n", d->points, m_affinity(gamemode) ? flags : "", d->frags, d->assists, d->deaths, sr.ratio, d->clientnum,
					m_team(gamemode, mutators) ? team : "", d->name,
						d == player1 ? " (you)" :
						d->priv == PRIV_MAX ? " (highest)" :
						d->priv == PRIV_ADMIN ? " (admin)" :
						d->priv == PRIV_MASTER ? " (master)" : "");
	}
	printf("\n");
}
